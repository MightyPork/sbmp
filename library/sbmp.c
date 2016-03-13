#include "sbmp.h"

#include <malloc.h>
#include <stdarg.h>
#include <string.h>
#include <inttypes.h>

#include "crc32.h"

/** Checksum field length */
#define CKSUM_LENGTH 4

/** Start of frame */
#define SBMP_FRM_START 0x01

// protos
static void reset_rx_state(SBMP_State *state);
static void reset_tx_state(SBMP_State *state);
static void call_frame_rx_callback(SBMP_State *state);
static void rx_cksum_begin(SBMP_State *state);
static void rx_cksum_update(SBMP_State *state, uint8_t byte);
static uint32_t rx_cksum_end(SBMP_State *state);
static bool rx_cksum_verify(SBMP_State *state, uint32_t received_cksum);
static void tx_cksum_begin(SBMP_State *state);
static void tx_cksum_update(SBMP_State *state, uint8_t byte);
static uint32_t tx_cksum_end(SBMP_State *state);


/**
 * @brief Error print function, can be replaced by custom one.
 * @param format : printf format
 * @param ...    : format substitutions
 */
void __attribute__((weak, format(printf, 1, 2)))
sbmp_error(const char* format, ...)
{
	printf("[SBMP] ");
	va_list va;
	va_start(va, format);
	vprintf(format, va);
	va_end(va);
	printf("\r\n");
}


// "packet state"
enum SBMP_RxParseState {
	PCK_STATE_IDLE,
	PCK_STATE_CKSUM_TYPE,
	PCK_STATE_LENGTH,
	PCK_STATE_HDRXOR,
	PCK_STATE_PAYLOAD,
	PCK_STATE_CKSUM,
};

/** Internal state of the SBMP node */
struct SBMP_State_struct {

	// --- reception ---

	uint32_t mb_buf; /*!< Multi-byte value buffer */
	uint8_t mb_cnt;

	uint8_t rx_hdr_xor; /*!< Header xor field */

	enum SBMP_RxParseState rx_state;

	uint8_t *rx_buffer;     /*!< Incoming packet buffer */
	size_t rx_buffer_i;   /*!< Buffer cursor */
	size_t rx_buffer_cap; /*!< Buffer capacity */

	size_t rx_length; /*!< Total payload length */

	SBMP_ChecksumType rx_cksum_type; /*!< Current packet's checksum type */

	uint32_t rx_crc_scratch; /*!< crc aggregation field for received data */

	void (*rx_handler)(uint8_t *payload, size_t length); /*!< Message received handler */

	// --- transmission ---

	size_t tx_remain; /*!< Number of bytes to transmit */
	SBMP_ChecksumType tx_cksum_type;
	uint32_t tx_crc_scratch; /*!< crc aggregation field for transmit */
	enum SBMP_RxParseState tx_state;

	// output functions. Only tx_func is needed.
	void (*tx_func)(uint8_t byte);  /*!< Function to send one byte */
	void (*tx_lock_func)(void);     /*!< Lock the serial interface tx (called before a frame) */
	void (*tx_release_func)(void);  /*!< Release the serial interface tx (called after a frame) */
};


/** Allocate the state struct & init all fields */
SBMP_State *sbmp_init(
		void (*rx_handler)(uint8_t *, size_t),
		void (*tx_func)(uint8_t),
		size_t buffer_size
) {
	SBMP_State *state = malloc(sizeof(SBMP_State));

	if (state == NULL) {
		// malloc failed
		sbmp_error("State malloc failed!");
		return NULL;
	}

	state->rx_handler = rx_handler;

	// Allocate the buffer
	state->rx_buffer = malloc(buffer_size);
	state->rx_buffer_cap = buffer_size;

	if (state->rx_buffer == NULL) {
		// malloc failed
		free(state);
		sbmp_error("Buffer malloc failed!");
		return NULL;
	}

	state->tx_func = tx_func;
	state->tx_lock_func = NULL;
	state->tx_release_func = NULL;

	reset_rx_state(state);
	reset_tx_state(state);

	return state;
}

/** Free all allocated memory */
void sbmp_destroy(SBMP_State *state)
{
	if (state == NULL) return;

	if (state->rx_buffer != NULL) {
		free(state->rx_buffer);
	}

	free(state);
}

/** Reset the receiver state  */
static void reset_rx_state(SBMP_State *state)
{
	state->rx_buffer_i = 0;
	state->rx_length = 0;
	state->mb_buf = 0;
	state->mb_cnt = 0;
	state->rx_hdr_xor = 0;
	state->rx_cksum_type = SBMP_CKSUM_NONE;
	state->rx_state = PCK_STATE_IDLE;
}

/** Reset the transmitter state */
static void reset_tx_state(SBMP_State *state)
{
	state->tx_state = PCK_STATE_IDLE;
	state->tx_remain = 0;
	state->tx_crc_scratch = 0;
	state->tx_cksum_type = SBMP_CKSUM_NONE;
}

/** Update a header XOR */
static inline
void hdrxor_update(SBMP_State *state, uint8_t rxbyte)
{
	state->rx_hdr_xor ^= rxbyte;
}

/** Check header xor against received value */
static inline
bool hdrxor_verify(SBMP_State *state, uint8_t rx_xor)
{
	return state->rx_hdr_xor == rx_xor;
}

/** Append a byte to the rx buffer */
static inline
void append_rx_byte(SBMP_State *state, uint8_t b)
{
	state->rx_buffer[state->rx_buffer_i++] = b;
}

/** Set n-th byte (0 = LSM) to a value */
static inline
void set_byte(uint32_t *acc, uint8_t pos, uint8_t byte)
{
	*acc |= (uint32_t)(byte << (pos * 8));
}

/**
 * Call the message handler with a copy of the payload,
 * clear the internal state.
 */
static void call_frame_rx_callback(SBMP_State *state)
{
	if (state->rx_handler == NULL) {
		sbmp_error("frame_handler is null!");
		goto done;
	}

	uint8_t *buf = malloc(state->rx_length);
	if (buf == NULL) {
		// malloc failed
		sbmp_error("Payload malloc failed!");
		goto done;
	}

	memcpy(buf, state->rx_buffer, state->rx_buffer_i);

	state->rx_handler(buf, state->rx_length);

done:
	reset_rx_state(state);
}

/**
 * @brief Receive a byte
 *
 * SOF 8 | CKSUM_TYPE 8 | LEN 16 | PAYLOAD | CKSUM 0/4
 *
 * @param state
 * @param rxbyte
 */
void sbmp_receive(SBMP_State *state, uint8_t rxbyte)
{
	switch (state->rx_state) {
		case PCK_STATE_IDLE:
			// can be first byte of a packet

			if (rxbyte == SBMP_FRM_START) {
				// next byte will be a checksum type

				hdrxor_update(state, rxbyte);

				// clean the parser state
				state->rx_cksum_type = SBMP_CKSUM_NONE;
				state->rx_state = PCK_STATE_CKSUM_TYPE;
			}
			break;

		case PCK_STATE_CKSUM_TYPE:
			state->rx_cksum_type = rxbyte; // checksum type received

			hdrxor_update(state, rxbyte);

			// next will be length
			state->rx_state = PCK_STATE_LENGTH;
			// clear MB for 2-byte length
			state->mb_buf = 0;
			state->mb_cnt = 0;
			break;

		case PCK_STATE_LENGTH:
			// append to the multi-byte buffer
			set_byte(&state->mb_buf, state->mb_cnt++, rxbyte);

			hdrxor_update(state, rxbyte);

			// if last of the MB field
			if (state->mb_cnt == 2) {

				// next will be the payload
				uint16_t len = (uint16_t) state->mb_buf;
				state->rx_length = len;

				if (len == 0) {
					sbmp_error("Rx packet with no payload!");
					reset_rx_state(state); // abort
					break;
				}

				state->rx_state = PCK_STATE_HDRXOR;
			}
			break;

		case PCK_STATE_HDRXOR:
			if (! hdrxor_verify(state, rxbyte)) {
				sbmp_error("Header XOR mismatch!");
				reset_rx_state(state); // abort
				break;
			}

			// Check if not too long
			if (state->rx_length > state->rx_buffer_cap) {
				sbmp_error("Rx packet too long - %"PRIu32"!", (uint32_t)state->rx_length);
				reset_rx_state(state); // abort
				break;
			}

			state->rx_state = PCK_STATE_PAYLOAD;
			rx_cksum_begin(state);
			break;

		case PCK_STATE_PAYLOAD:
			append_rx_byte(state, rxbyte);
			rx_cksum_update(state, rxbyte);

			if (state->rx_buffer_i == state->rx_length) {
				// payload rx complete

				if (state->rx_cksum_type != SBMP_CKSUM_NONE) {
					// receive the checksum
					state->rx_state = PCK_STATE_CKSUM;

					// clear MB for 4-byte length
					state->mb_buf = 0;
					state->mb_cnt = 0;
				} else {
					// no checksum
					// fire the callback
					call_frame_rx_callback(state);
				}
			}
			break;

		case PCK_STATE_CKSUM:
			// append to the multi-byte buffer
			set_byte(&state->mb_buf, state->mb_cnt++, rxbyte);

			// if last of the MB field
			if (state->mb_cnt == 4) {

				if (rx_cksum_verify(state, state->mb_buf)) {
					call_frame_rx_callback(state);
				} else {
					sbmp_error("Rx checksum mismatch!");
				}

				// end of the packet
				state->rx_state = PCK_STATE_IDLE;
			}
			break;
	}
}

// --- Functions for calculating a SBMP checksum ---

/** Start calculating a checksum */
static void rx_cksum_begin(SBMP_State *state)
{
	if (state->rx_cksum_type == SBMP_CKSUM_CRC32) {
		state->rx_crc_scratch = crc32_begin();
	}
}

/** Update the checksum calculation with an incoming byte */
static void rx_cksum_update(SBMP_State *state, uint8_t byte)
{
	if (state->rx_cksum_type == SBMP_CKSUM_CRC32) {
		crc32_update(&state->rx_crc_scratch, byte);
	}
}

/** Stop the checksum calculation, get the result */
static uint32_t rx_cksum_end(SBMP_State *state)
{
	if (state->rx_cksum_type == SBMP_CKSUM_CRC32) {
		return crc32_end(state->rx_crc_scratch);
	}

	return 0;
}

/** Check if the calculated checksum matches the received one */
static bool rx_cksum_verify(SBMP_State *state, uint32_t received_cksum)
{
	uint32_t computed = rx_cksum_end(state);

	if (state->rx_cksum_type == SBMP_CKSUM_CRC32) {
		return (computed == received_cksum);
	} else {
		// unknown checksum type
		return true; // assume it's OK
	}
}

/** Start calculating a checksum */
static void tx_cksum_begin(SBMP_State *state)
{
	if (state->tx_cksum_type == SBMP_CKSUM_CRC32) {
		state->tx_crc_scratch = crc32_begin();
	}
}

/** Update the checksum calculation with an incoming byte */
static void tx_cksum_update(SBMP_State *state, uint8_t byte)
{
	if (state->tx_cksum_type == SBMP_CKSUM_CRC32) {
		crc32_update(&state->tx_crc_scratch, byte);
	}
}

/** Stop the checksum calculation, get the result */
static uint32_t tx_cksum_end(SBMP_State *state)
{
	if (state->tx_cksum_type == SBMP_CKSUM_CRC32) {
		return crc32_end(state->tx_crc_scratch);
	}

	return 0;
}

// ---------------------------------------------------

bool sbmp_transmit_start(SBMP_State *state, SBMP_ChecksumType cksum_type, size_t length)
{
	if (state->tx_state != PCK_STATE_IDLE) return false;

	if (length > 0xFFFF) {
		sbmp_error("Tx packet too long.");
		return false;
	}

	if (state->tx_func == NULL) {
		sbmp_error("No tx func!");
		return false;
	}

	reset_tx_state(state);

	state->tx_cksum_type = cksum_type;
	state->tx_remain = length;
	state->tx_state = PCK_STATE_PAYLOAD;

	if (state->tx_lock_func) {
		state->tx_lock_func();
	}

	// Send the header

	uint16_t len = (uint16_t) length;

	uint8_t hdr[4] = {
		0x01,
		cksum_type,
		len & 0xFF,
		(len >> 8) & 0xFF
	};

	uint8_t hdr_xor = 0;
	for (int i = 0; i < 4; i++) {
		hdr_xor ^= hdr[i];
		state->tx_func(hdr[i]);
	}

	state->tx_func(hdr_xor);

	tx_cksum_begin(state);

	return true;
}


void sbmp_transmit_byte(SBMP_State *state, uint8_t byte)
{
	state->tx_func(byte);
	tx_cksum_update(state, byte);
	state->tx_remain--;

	if (state->tx_remain == 0) {
		if (state->tx_cksum_type != SBMP_CKSUM_NONE) {
			uint32_t cksum = tx_cksum_end(state);

			// send the checksum
			state->tx_func(cksum & 0xFF);
			state->tx_func((cksum >> 8) & 0xFF);
			state->tx_func((cksum >> 16) & 0xFF);
			state->tx_func((cksum >> 24) & 0xFF);
		}

		if (state->tx_release_func) {
			state->tx_release_func();
		}

		state->tx_state = PCK_STATE_IDLE; // tx done
	}
}


size_t sbmp_transmit_buffer(SBMP_State *state, const uint8_t *buffer, size_t length)
{
	size_t remain = length;
	while (state->tx_state == PCK_STATE_PAYLOAD && remain-- > 0) {
		sbmp_transmit_byte(state, *buffer++);
	}

	return (length - remain);
}


// --- Higher level of the protocol ------------------

SBMP_Datagram *sbmp_parse_datagram(uint8_t *payload, size_t length)
{
	if (length < 3) {
		return NULL; // shorter than the minimal no-payload datagram
	}

	SBMP_Datagram *dg = malloc(sizeof(SBMP_Datagram));

	// S.N. (2 B) | Dg Type (1 B) | Payload

	dg->_backing_buffer = payload;
	dg->session_number = (uint16_t)((payload[0]) | (payload[1] << 8));
	dg->datagram_type = payload[2];
	dg->datagram_length = length - 3;
	dg->datagram = payload + 3; // pointer arith

	return dg;
}


void sbmp_destroy_datagram(SBMP_Datagram *dg)
{
	if (dg == NULL) return;

	if (dg->_backing_buffer != NULL) {
		// free the payload buffer
		free(dg->_backing_buffer);
	}

	// free the DG itself
	free(dg);
}

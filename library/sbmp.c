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
static void sbmp_reset(SBMP_State *state);
static void rx_done(SBMP_State *state);
static void cksum_begin(SBMP_State *state);
static void cksum_update(SBMP_State *state, uint8_t byte);
static uint32_t cksum_end(SBMP_State *state);
static bool cksum_verify(SBMP_State *state, uint32_t received_cksum);


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



enum SBMP_RxParseState {
	PCK_STATE_IDLE,
	PCK_STATE_CKSUM_TYPE,
	PCK_STATE_LENGTH,
	PCK_STATE_HDRXOR,
	PCK_STATE_PAYLOAD,
	PCK_STATE_PAYLOAD_DISCARD, // if bad checksum type
	PCK_STATE_CKSUM,
};

/** Internal state of the SBMP node */
struct SBMP_State_struct {
	// for parsing multi-byte values
	uint32_t mb_buf;
	uint8_t mb_cnt;

	uint8_t hdr_xor; /*!< Header xor field */

	enum SBMP_RxParseState parse_state;

	uint8_t *rx_buffer;     /*!< Incoming packet buffer */
	uint16_t rx_buffer_i;   /*!< Buffer cursor */
	uint16_t rx_buffer_cap; /*!< Buffer capacity */

	uint16_t payload_length; /*!< Total payload length */

	SBMP_ChecksumType cksum_type; /*!< Current packet's checksum type */

	uint32_t crc_old; /*!< crc aggregation field */

	void (*msg_handler)(uint8_t *payload, uint16_t length); /*!< Message received handler */
};


/** Allocate the state struct & init all fields */
SBMP_State *sbmp_init(void(*msg_handler)(uint8_t *, uint16_t), uint16_t buffer_size)
{
	SBMP_State *state = malloc(sizeof(SBMP_State));

	if (state == NULL) {
		// malloc failed
		sbmp_error("State malloc failed!");
		return NULL;
	}

	state->msg_handler = msg_handler;

	// Allocate the buffer
	state->rx_buffer = malloc(buffer_size);
	state->rx_buffer_cap = buffer_size;

	if (state->rx_buffer == NULL) {
		// malloc failed
		free(state);
		sbmp_error("Buffer malloc failed!");
		return NULL;
	}

	sbmp_reset(state);

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
static void sbmp_reset(SBMP_State *state)
{
	state->rx_buffer_i = 0;
	state->payload_length = 0;
	state->mb_buf = 0;
	state->mb_cnt = 0;
	state->hdr_xor = 0;
	state->cksum_type = SBMP_CKSUM_NONE;
	state->parse_state = PCK_STATE_IDLE;
}

/** Update a header XOR */
static inline
void hdrxor_update(SBMP_State *state, uint8_t rxbyte)
{
	state->hdr_xor ^= rxbyte;
}

/** Check header xor against received value */
static inline
bool hdrxor_verify(SBMP_State *state, uint8_t rx_xor)
{
	return state->hdr_xor == rx_xor;
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
static void rx_done(SBMP_State *state)
{
	if (state->msg_handler == NULL) {
		sbmp_error("msg_handler is null!");
		goto done;
	}

	uint8_t *buf = malloc(state->payload_length);
	if (buf == NULL) {
		// malloc failed
		sbmp_error("Payload malloc failed!");
		goto done;
	}

	memcpy(buf, state->rx_buffer, state->rx_buffer_i);

	state->msg_handler(buf, state->payload_length);

done:
	sbmp_reset(state);
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
	switch (state->parse_state) {
		case PCK_STATE_IDLE:
			// can be first byte of a packet

			if (rxbyte == SBMP_FRM_START) {
				// next byte will be a checksum type

				hdrxor_update(state, rxbyte);

				// clean the parser state
				state->cksum_type = SBMP_CKSUM_NONE;
				state->parse_state = PCK_STATE_CKSUM_TYPE;
			}
			break;

		case PCK_STATE_CKSUM_TYPE:
			state->cksum_type = rxbyte; // checksum type received

			hdrxor_update(state, rxbyte);

			// next will be length
			state->parse_state = PCK_STATE_LENGTH;
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
				state->payload_length = len;

				if (len == 0) {
					sbmp_error("Rx packet with no payload!");
					sbmp_reset(state); // abort
					break;
				}

				state->parse_state = PCK_STATE_HDRXOR;
			}
			break;

		case PCK_STATE_HDRXOR:
			if (! hdrxor_verify(state, rxbyte)) {
				sbmp_error("Header XOR mismatch!");
				sbmp_reset(state); // abort
				break;
			}

			// Check if not too long
			if (state->payload_length > state->rx_buffer_cap) {
				sbmp_error("Rx packet too long - %"PRIu16"!", state->payload_length);
				sbmp_reset(state); // abort
				break;
			}

			state->parse_state = PCK_STATE_PAYLOAD;
			cksum_begin(state);
			break;

		case PCK_STATE_PAYLOAD:
			append_rx_byte(state, rxbyte);
			cksum_update(state, rxbyte);

			if (state->rx_buffer_i == state->payload_length) {
				// payload rx complete

				if (state->cksum_type != SBMP_CKSUM_NONE) {
					// receive the checksum
					state->parse_state = PCK_STATE_CKSUM;

					// clear MB for 4-byte length
					state->mb_buf = 0;
					state->mb_cnt = 0;
				} else {
					// no checksum
					// fire the callback
					rx_done(state);
				}
			}
			break;

		case PCK_STATE_PAYLOAD_DISCARD:
			if (state->rx_buffer_i == state->payload_length) {
				// payload received fully
				state->parse_state = PCK_STATE_IDLE;
			}
			break;

		case PCK_STATE_CKSUM:
			// append to the multi-byte buffer
			set_byte(&state->mb_buf, state->mb_cnt++, rxbyte);

			// if last of the MB field
			if (state->mb_cnt == 4) {

				if (cksum_verify(state, state->mb_buf)) {
					rx_done(state); // call the callback fn
				} else {
					sbmp_error("Rx checksum mismatch!");
				}

				// end of the packet
				state->parse_state = PCK_STATE_IDLE;
			}
			break;
	}
}


// --- Functions for calculating a SBMP checksum ---

/** Start calculating a checksum */
static void cksum_begin(SBMP_State *state)
{
	if (state->cksum_type == SBMP_CKSUM_CRC32) {
		state->crc_old = crc32_begin();
	}
}

/** Update the checksum calculation with an incoming byte */
static void cksum_update(SBMP_State *state, uint8_t byte)
{
	if (state->cksum_type == SBMP_CKSUM_CRC32) {
		crc32_update(&state->crc_old, byte);
	}
}

/** Stop the checksum calculation, get the result */
static uint32_t cksum_end(SBMP_State *state)
{
	if (state->cksum_type == SBMP_CKSUM_CRC32) {
		return crc32_end(state->crc_old);
	}

	return 0;
}

/** Check if the calculated checksum matches the received one */
static bool cksum_verify(SBMP_State *state, uint32_t received_cksum)
{
	uint32_t computed = cksum_end(state);

	if (state->cksum_type == SBMP_CKSUM_CRC32) {
		return (computed == received_cksum);
	} else {
		// unknown checksum type
		return true; // assume it's OK
	}
}

// ---------------------------------------------------

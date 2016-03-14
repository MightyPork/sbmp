#include <stdio.h>
#include <stdarg.h>
#include <inttypes.h>

#include "sbmp.h"
#include "crc32.h"

// protos
static void reset_rx_state(SBMP_FrmState *state);
static void reset_tx_state(SBMP_FrmState *state);
static void call_frame_rx_callback(SBMP_FrmState *state);
static void rx_cksum_begin(SBMP_FrmState *state);
static void rx_cksum_update(SBMP_FrmState *state, uint8_t byte);
static uint32_t rx_cksum_end(SBMP_FrmState *state);
static bool rx_cksum_verify(SBMP_FrmState *state, uint32_t received_cksum);
static void tx_cksum_begin(SBMP_FrmState *state);
static void tx_cksum_update(SBMP_FrmState *state, uint8_t byte);
static uint32_t tx_cksum_end(SBMP_FrmState *state);


/** Allocate the state struct & init all fields */
SBMP_FrmState *sbmp_frm_init(
	SBMP_FrmState *state,
	uint8_t *buffer, uint16_t buffer_size,
	void (*rx_handler)(uint8_t *, uint16_t, void *),
	void (*tx_func)(uint8_t)
)
{
	if (state == NULL) {
		// caller wants us to allocate it
		state = malloc(sizeof(SBMP_FrmState));
	}

	state->rx_handler = rx_handler;

	if (buffer == NULL) {
		// caller wants us to allocate it
		buffer = malloc(buffer_size);
	}

	state->rx_buffer = buffer;
	state->rx_buffer_cap = buffer_size;

	state->user_token = NULL; // NULL if not set

	state->tx_func = tx_func;
	state->tx_lock_func = NULL;
	state->tx_release_func = NULL;

	sbmp_frm_reset(state);

	return state;
}

/** Reset the internal state */
void sbmp_frm_reset(SBMP_FrmState *state)
{
	reset_rx_state(state);
	reset_tx_state(state);
}

/** Reset the receiver state  */
static void reset_rx_state(SBMP_FrmState *state)
{
	state->rx_buffer_i = 0;
	state->rx_length = 0;
	state->mb_buf = 0;
	state->mb_cnt = 0;
	state->rx_hdr_xor = 0;
	state->rx_crc_scratch = 0;
	state->rx_cksum_type = SBMP_CKSUM_NONE;
	state->rx_state = FRM_STATE_IDLE;
//	printf("---- RX RESET STATE ----\n");
}

/** Reset the transmitter state */
static void reset_tx_state(SBMP_FrmState *state)
{
	state->tx_state = FRM_STATE_IDLE;
	state->tx_remain = 0;
	state->tx_crc_scratch = 0;
	state->tx_cksum_type = SBMP_CKSUM_NONE;
//	printf("---- TX RESET STATE ----\n");
}

/** Update a header XOR */
static inline
void hdrxor_update(SBMP_FrmState *state, uint8_t rxbyte)
{
	state->rx_hdr_xor ^= rxbyte;
}

/** Check header xor against received value */
static inline
bool hdrxor_verify(SBMP_FrmState *state, uint8_t rx_xor)
{
	return state->rx_hdr_xor == rx_xor;
}

/** Append a byte to the rx buffer */
static inline
void append_rx_byte(SBMP_FrmState *state, uint8_t b)
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
 * Call the message handler with the payload.
 *
 */
static void call_frame_rx_callback(SBMP_FrmState *state)
{
	if (state->rx_handler == NULL) {
		sbmp_error("frame_handler is null!");
		return;
	}

	state->rx_state = FRM_STATE_WAIT_HANDLER;
	state->rx_handler(state->rx_buffer, state->rx_length, state->user_token);
}

/**
 * @brief Receive a byte
 *
 * SOF 8 | CKSUM_TYPE 8 | LEN 16 | PAYLOAD | CKSUM 0/4
 *
 * @param state
 * @param rxbyte
 * @return true if the byte was consumed.
 */
SBMP_RxStatus sbmp_receive(SBMP_FrmState *state, uint8_t rxbyte)
{
	SBMP_RxStatus retval = SBMP_RX_OK;

	switch (state->rx_state) {
		case FRM_STATE_WAIT_HANDLER:
			retval = SBMP_RX_BUSY;
			break;

		case FRM_STATE_IDLE:
			// can be first byte of a packet

			if (rxbyte == 0x01) { // start byte

				hdrxor_update(state, rxbyte);

				state->rx_state = FRM_STATE_CKSUM_TYPE;
			} else {
				// bad char
				retval = SBMP_RX_INVALID;
			}
			break;

		case FRM_STATE_CKSUM_TYPE:
			state->rx_cksum_type = rxbyte; // checksum type received

			hdrxor_update(state, rxbyte);

			// next will be length
			state->rx_state = FRM_STATE_LENGTH;
			// clear MB for 2-byte length
			state->mb_buf = 0;
			state->mb_cnt = 0;
			break;

		case FRM_STATE_LENGTH:
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

				state->rx_state = FRM_STATE_HDRXOR;
			}
			break;

		case FRM_STATE_HDRXOR:
			if (! hdrxor_verify(state, rxbyte)) {
				sbmp_error("Header XOR mismatch!");
				reset_rx_state(state); // abort
				break;
			}

			// Check if not too long
			if (state->rx_length > state->rx_buffer_cap) {
				sbmp_error("Rx packet too long - %"PRIu16"!", (uint16_t)state->rx_length);
				reset_rx_state(state); // abort
				break;
			}

			state->rx_state = FRM_STATE_PAYLOAD;
			rx_cksum_begin(state);
			break;

		case FRM_STATE_PAYLOAD:
			append_rx_byte(state, rxbyte);
			rx_cksum_update(state, rxbyte);

			if (state->rx_buffer_i == state->rx_length) {
				// payload rx complete

				if (state->rx_cksum_type != SBMP_CKSUM_NONE) {
					// receive the checksum
					state->rx_state = FRM_STATE_CKSUM;

					// clear MB for the 4-byte length
					state->mb_buf = 0;
					state->mb_cnt = 0;
				} else {
					// no checksum
					// fire the callback
					call_frame_rx_callback(state);

					// clear
					reset_rx_state(state);
				}
			}
			break;

		case FRM_STATE_CKSUM:
			// append to the multi-byte buffer
			set_byte(&state->mb_buf, state->mb_cnt++, rxbyte);

			// if last of the MB field
			if (state->mb_cnt == 4) {

				if (rx_cksum_verify(state, state->mb_buf)) {
					call_frame_rx_callback(state);
				} else {
					sbmp_error("Rx checksum mismatch!");
				}

				// clear, enter IDLE
				reset_rx_state(state);
			}
			break;
	}

	return retval;
}

// --- Functions for calculating a SBMP checksum ---

/** Start calculating a checksum */
static void rx_cksum_begin(SBMP_FrmState *state)
{
	if (state->rx_cksum_type == SBMP_CKSUM_CRC32) {
		state->rx_crc_scratch = crc32_begin();
	}
}

/** Update the checksum calculation with an incoming byte */
static void rx_cksum_update(SBMP_FrmState *state, uint8_t byte)
{
	if (state->rx_cksum_type == SBMP_CKSUM_CRC32) {
		crc32_update(&state->rx_crc_scratch, byte);
	}
}

/** Stop the checksum calculation, get the result */
static uint32_t rx_cksum_end(SBMP_FrmState *state)
{
	if (state->rx_cksum_type == SBMP_CKSUM_CRC32) {
		return crc32_end(state->rx_crc_scratch);
	}

	return 0;
}

/** Check if the calculated checksum matches the received one */
static bool rx_cksum_verify(SBMP_FrmState *state, uint32_t received_cksum)
{
	uint32_t computed = rx_cksum_end(state);

	if (state->rx_cksum_type == SBMP_CKSUM_CRC32) {
		return (computed == received_cksum);
	}

	// unknown checksum type
	return true; // assume it's OK
}

/** Start calculating a checksum */
static void tx_cksum_begin(SBMP_FrmState *state)
{
	if (state->tx_cksum_type == SBMP_CKSUM_CRC32) {
		state->tx_crc_scratch = crc32_begin();
	}
}

/** Update the checksum calculation with an incoming byte */
static void tx_cksum_update(SBMP_FrmState *state, uint8_t byte)
{
	if (state->tx_cksum_type == SBMP_CKSUM_CRC32) {
		crc32_update(&state->tx_crc_scratch, byte);
	}
}

/** Stop the checksum calculation, get the result */
static uint32_t tx_cksum_end(SBMP_FrmState *state)
{
	if (state->tx_cksum_type == SBMP_CKSUM_CRC32) {
		return crc32_end(state->tx_crc_scratch);
	}
	return 0;
}

// ---------------------------------------------------

bool sbmp_start_frame(SBMP_FrmState *state, SBMP_ChecksumType cksum_type, uint16_t length)
{
	if (state->tx_state != FRM_STATE_IDLE) {
		sbmp_error("Tx busy.");
		return false;
	}

	if (state->tx_func == NULL) {
		sbmp_error("No tx func!");
		return false;
	}

	reset_tx_state(state);

	state->tx_cksum_type = cksum_type;
	state->tx_remain = length;
	state->tx_state = FRM_STATE_PAYLOAD;

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

/** Send a byte in the currently open frame */
bool sbmp_send_byte(SBMP_FrmState *state, uint8_t byte)
{
	if (state->tx_remain == 0 || state->tx_state != FRM_STATE_PAYLOAD) {
		return false;
	}

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

		state->tx_state = FRM_STATE_IDLE; // tx done
	}

	return true;
}

/** Send a buffer in the currently open frame */
uint16_t sbmp_send_buffer(SBMP_FrmState *state, const uint8_t *buffer, uint16_t length)
{
	if (state->tx_remain == 0 || state->tx_state != FRM_STATE_PAYLOAD) {
		return false;
	}

	uint16_t remain = length;
	while (state->tx_state == FRM_STATE_PAYLOAD && remain-- > 0) {
		if (! sbmp_send_byte(state, *buffer++)) {
			remain++; // "push back"
			break;
		}
	}

	return (length - remain);
}

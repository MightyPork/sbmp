#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <inttypes.h>

#include "sbmp_config.h"
#include "sbmp_logging.h"
#include "sbmp_checksum.h"
#include "sbmp_frame.h"

// protos
static void reset_rx_state(SBMP_FrmState *state);
static void reset_tx_state(SBMP_FrmState *state);
static void call_frame_rx_callback(SBMP_FrmState *state);


/** Allocate the state struct & init all fields */
SBMP_FrmState *sbmp_frm_init(
	SBMP_FrmState *state,
	uint8_t *buffer, uint16_t buffer_size,
	void (*rx_handler)(uint8_t *, uint16_t, void *),
	void (*tx_func)(uint8_t))
{

#if SBMP_MALLOC

	if (state == NULL) {
		// caller wants us to allocate it
		state = malloc(sizeof(SBMP_FrmState));
	}

	if (buffer == NULL) {
		// caller wants us to allocate it
		buffer = malloc(buffer_size);
	}

#else

	if (state == NULL || buffer == NULL) {
		return NULL; // malloc not enabled, fail
	}

#endif

	state->rx_handler = rx_handler;

	state->rx_buffer = buffer;
	state->rx_buffer_cap = buffer_size;

	state->user_token = NULL; // NULL if not set

	state->tx_func = tx_func;

	state->enabled = false;

	sbmp_frm_reset(state);

	return state;
}

/** Reset the internal state */
void sbmp_frm_reset(SBMP_FrmState *state)
{
	reset_rx_state(state);
	reset_tx_state(state);
}

/** Enable or disable */
void sbmp_frm_enable(SBMP_FrmState *state, bool enable)
{
	state->enabled = enable;
}

/** Set user token */
void sbmp_frm_set_user_token(SBMP_FrmState *state, void *token)
{
	state->user_token = token;
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
 * @return status
 */
SBMP_RxStatus sbmp_frm_receive(SBMP_FrmState *state, uint8_t rxbyte)
{
	if (!state->enabled) {
		return SBMP_RX_DISABLED;
	}

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
				// discard the rest + checksum
				state->rx_state = FRM_STATE_DISCARD;
				state->rx_length += checksum_length(state->rx_cksum_type);
				break;
			}

			state->rx_state = FRM_STATE_PAYLOAD;
			cksum_begin(state->rx_cksum_type, &state->rx_crc_scratch);
			break;

		case FRM_STATE_DISCARD:
			state->rx_buffer_i++;
			if (state->rx_buffer_i == state->rx_length) {
				// done
				reset_rx_state(state); // go IDLE
			}
			break;

		case FRM_STATE_PAYLOAD:
			append_rx_byte(state, rxbyte);
			cksum_update(state->rx_cksum_type, &state->rx_crc_scratch, rxbyte);

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
			if (state->mb_cnt == checksum_length(state->rx_cksum_type)) {

				if (cksum_verify(state->rx_cksum_type, &state->rx_crc_scratch, state->mb_buf)) {
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

/** Send a frame header */
bool sbmp_start_frame(SBMP_FrmState *state, SBMP_ChecksumType cksum_type, uint16_t length)
{
	if (!state->enabled) {
		sbmp_error("Can't tx, not enabled.");
		return false;
	}

	if (state->tx_state != FRM_STATE_IDLE) {
		sbmp_error("Can't tx, busy.");
		return false;
	}

	if (state->tx_func == NULL) {
		sbmp_error("Can't tx, no tx func!");
		return false;
	}

	if (cksum_type == SBMP_CKSUM_CRC32 && !SBMP_SUPPORT_CRC32) {
		sbmp_error("CRC32 disabled, using XOR for Tx.");
		cksum_type = SBMP_CKSUM_XOR;
	}

	reset_tx_state(state);

	state->tx_cksum_type = cksum_type;
	state->tx_remain = length;
	state->tx_state = FRM_STATE_PAYLOAD;

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

	cksum_begin(state->tx_cksum_type, &state->tx_crc_scratch);

	return true;
}

/** End frame and enter idle mode */
static void end_frame(SBMP_FrmState *state)
{
	cksum_end(state->tx_cksum_type, &state->tx_crc_scratch);

	uint32_t cksum = state->tx_crc_scratch;

	switch (state->tx_cksum_type) {
		case SBMP_CKSUM_NONE:
			break; // do nothing

		case SBMP_CKSUM_XOR:
			// 1-byte checksum
			state->tx_func(cksum & 0xFF);
			break;

		case SBMP_CKSUM_CRC32:
			state->tx_func(cksum & 0xFF);
			state->tx_func((cksum >> 8) & 0xFF);
			state->tx_func((cksum >> 16) & 0xFF);
			state->tx_func((cksum >> 24) & 0xFF);
	}

	state->tx_state = FRM_STATE_IDLE; // tx done
}

/** Send a byte in the currently open frame */
bool sbmp_send_byte(SBMP_FrmState *state, uint8_t byte)
{
	if (!state->enabled) {
		sbmp_error("Can't tx, not enabled.");
		return false;
	}

	if (state->tx_remain == 0 || state->tx_state != FRM_STATE_PAYLOAD) {
		return false;
	}

	state->tx_func(byte);
	cksum_update(state->tx_cksum_type, &state->tx_crc_scratch, byte);
	state->tx_remain--;

	if (state->tx_remain == 0) {
		end_frame(state); // checksum & go idle
	}

	return true;
}

/** Send a buffer in the currently open frame */
uint16_t sbmp_send_buffer(SBMP_FrmState *state, const uint8_t *buffer, uint16_t length)
{
	if (!state->enabled) {
		sbmp_error("Can't tx, not enabled.");
		return false;
	}

	if (state->tx_state != FRM_STATE_PAYLOAD) {
		return false; // invalid call
	}

	if (length == 0) {
		end_frame(state); // checksum & go idle
		return 0;
	}

	if (state->tx_remain == 0) {
		return false; // write past EOF (this shouldn't happen)
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

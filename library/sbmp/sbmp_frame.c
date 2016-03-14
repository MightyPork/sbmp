#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <inttypes.h>

#include "sbmp_config.h"
#include "sbmp_logging.h"
#include "sbmp_checksum.h"
#include "sbmp_frame.h"

// protos
static void call_frame_rx_callback(SBMP_FrmInst *frm);


/** Allocate the state struct & init all fields */
SBMP_FrmInst *sbmp_frm_init(
	SBMP_FrmInst *frm,
	uint8_t *buffer,
	uint16_t buffer_size,
	void (*rx_handler)(uint8_t *, uint16_t, void *),
	void (*tx_func)(uint8_t))
{

#if SBMP_MALLOC

	if (frm == NULL) {
		// caller wants us to allocate it
		frm = malloc(sizeof(SBMP_FrmInst));
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

	frm->rx_handler = rx_handler;

	frm->rx_buffer = buffer;
	frm->rx_buffer_cap = buffer_size;

	frm->user_token = NULL; // NULL if not set

	frm->tx_func = tx_func;

	frm->rx_enabled = false;
	frm->tx_enabled = false;

	sbmp_frm_reset(frm);

	return frm;
}

/** Reset the internal state */
void sbmp_frm_reset(SBMP_FrmInst *frm)
{
	sbmp_frm_reset_rx(frm);
	sbmp_frm_reset_tx(frm);
}

/** Enable or disable Rx */
void sbmp_frm_enable_rx(SBMP_FrmInst *frm, bool enable)
{
	frm->rx_enabled = enable;
}

/** Enable or disable Tx */
void sbmp_frm_enable_tx(SBMP_FrmInst *frm, bool enable)
{
	frm->tx_enabled = enable;
}

/** Enable or disable both Rx and Tx */
void sbmp_frm_enable(SBMP_FrmInst *frm, bool enable)
{
	sbmp_frm_enable_rx(frm, enable);
	sbmp_frm_enable_tx(frm, enable);
}

/** Set user token */
void sbmp_frm_set_user_token(SBMP_FrmInst *frm, void *token)
{
	frm->user_token = token;
}

/** Reset the receiver state  */
void sbmp_frm_reset_rx(SBMP_FrmInst *frm)
{
	frm->rx_buffer_i = 0;
	frm->rx_length = 0;
	frm->mb_buf = 0;
	frm->mb_cnt = 0;
	frm->rx_hdr_xor = 0;
	frm->rx_cksum_scratch = 0;
	frm->rx_cksum_type = SBMP_CKSUM_NONE;
	frm->rx_status = FRM_STATE_IDLE;
//	printf("---- RX RESET STATE ----\n");
}

/** Reset the transmitter state */
void sbmp_frm_reset_tx(SBMP_FrmInst *frm)
{
	frm->tx_status = FRM_STATE_IDLE;
	frm->tx_remain = 0;
	frm->tx_cksum_scratch = 0;
	frm->tx_cksum_type = SBMP_CKSUM_NONE;
//	printf("---- TX RESET STATE ----\n");
}

/** Update a header XOR */
static inline
void hdrxor_update(SBMP_FrmInst *frm, uint8_t rxbyte)
{
	frm->rx_hdr_xor ^= rxbyte;
}

/** Check header xor against received value */
static inline
bool hdrxor_verify(SBMP_FrmInst *frm, uint8_t rx_xor)
{
	return frm->rx_hdr_xor == rx_xor;
}

/** Append a byte to the rx buffer */
static inline
void append_rx_byte(SBMP_FrmInst *frm, uint8_t b)
{
	frm->rx_buffer[frm->rx_buffer_i++] = b;
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
static void call_frame_rx_callback(SBMP_FrmInst *frm)
{
	if (frm->rx_handler == NULL) {
		sbmp_error("frame_handler is null!");
		return;
	}

	frm->rx_status = FRM_STATE_WAIT_HANDLER;
	frm->rx_handler(frm->rx_buffer, frm->rx_length, frm->user_token);
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
SBMP_RxStatus sbmp_frm_receive(SBMP_FrmInst *frm, uint8_t rxbyte)
{
	if (! frm->rx_enabled) {
		return SBMP_RX_DISABLED;
	}

	SBMP_RxStatus retval = SBMP_RX_OK;

	switch (frm->rx_status) {
		case FRM_STATE_WAIT_HANDLER:
			retval = SBMP_RX_BUSY;
			break;

		case FRM_STATE_IDLE:
			// can be first byte of a packet

			if (rxbyte == 0x01) { // start byte

				hdrxor_update(frm, rxbyte);

				frm->rx_status = FRM_STATE_CKSUM_TYPE;
			} else {
				// bad char
				retval = SBMP_RX_INVALID;
			}
			break;

		case FRM_STATE_CKSUM_TYPE:
			frm->rx_cksum_type = rxbyte; // checksum type received

			hdrxor_update(frm, rxbyte);

			// next will be length
			frm->rx_status = FRM_STATE_LENGTH;
			// clear MB for 2-byte length
			frm->mb_buf = 0;
			frm->mb_cnt = 0;
			break;

		case FRM_STATE_LENGTH:
			// append to the multi-byte buffer
			set_byte(&frm->mb_buf, frm->mb_cnt++, rxbyte);

			hdrxor_update(frm, rxbyte);

			// if last of the MB field
			if (frm->mb_cnt == 2) {

				// next will be the payload
				uint16_t len = (uint16_t) frm->mb_buf;
				frm->rx_length = len;

				if (len == 0) {
					sbmp_error("Rx packet with no payload!");
					sbmp_frm_reset_rx(frm); // abort
					break;
				}

				frm->rx_status = FRM_STATE_HDRXOR;
			}
			break;

		case FRM_STATE_HDRXOR:
			if (! hdrxor_verify(frm, rxbyte)) {
				sbmp_error("Header XOR mismatch!");
				sbmp_frm_reset_rx(frm); // abort
				break;
			}

			// Check if not too long
			if (frm->rx_length > frm->rx_buffer_cap) {
				sbmp_error("Rx packet too long - %"PRIu16"!", (uint16_t)frm->rx_length);
				// discard the rest + checksum
				frm->rx_status = FRM_STATE_DISCARD;
				frm->rx_length += chksum_length(frm->rx_cksum_type);
				break;
			}

			frm->rx_status = FRM_STATE_PAYLOAD;
			cksum_begin(frm->rx_cksum_type, &frm->rx_cksum_scratch);
			break;

		case FRM_STATE_DISCARD:
			frm->rx_buffer_i++;
			if (frm->rx_buffer_i == frm->rx_length) {
				// done
				sbmp_frm_reset_rx(frm); // go IDLE
			}
			break;

		case FRM_STATE_PAYLOAD:
			append_rx_byte(frm, rxbyte);
			cksum_update(frm->rx_cksum_type, &frm->rx_cksum_scratch, rxbyte);

			if (frm->rx_buffer_i == frm->rx_length) {
				// payload rx complete

				if (frm->rx_cksum_type != SBMP_CKSUM_NONE) {
					// receive the checksum
					frm->rx_status = FRM_STATE_CKSUM;

					// clear MB for the 4-byte length
					frm->mb_buf = 0;
					frm->mb_cnt = 0;
				} else {
					// no checksum
					// fire the callback
					call_frame_rx_callback(frm);

					// clear
					sbmp_frm_reset_rx(frm);
				}
			}
			break;

		case FRM_STATE_CKSUM:
			// append to the multi-byte buffer
			set_byte(&frm->mb_buf, frm->mb_cnt++, rxbyte);

			// if last of the MB field
			if (frm->mb_cnt == chksum_length(frm->rx_cksum_type)) {

				if (cksum_verify(frm->rx_cksum_type, &frm->rx_cksum_scratch, frm->mb_buf)) {
					call_frame_rx_callback(frm);
				} else {
					sbmp_error("Rx checksum mismatch!");
				}

				// clear, enter IDLE
				sbmp_frm_reset_rx(frm);
			}
			break;
	}

	return retval;
}

/** Send a frame header */
bool sbmp_frm_start(SBMP_FrmInst *frm, SBMP_CksumType cksum_type, uint16_t length)
{
	if (! frm->tx_enabled) {
		sbmp_error("Can't tx, not enabled.");
		return false;
	}

	if (frm->tx_status != FRM_STATE_IDLE) {
		sbmp_error("Can't tx, busy.");
		return false;
	}

	if (frm->tx_func == NULL) {
		sbmp_error("Can't tx, no tx func!");
		return false;
	}

	if (cksum_type == SBMP_CKSUM_CRC32 && !SBMP_HAS_CRC32) {
		sbmp_error("CRC32 disabled, using XOR for Tx.");
		cksum_type = SBMP_CKSUM_XOR;
	}

	sbmp_frm_reset_tx(frm);

	frm->tx_cksum_type = cksum_type;
	frm->tx_remain = length;
	frm->tx_status = FRM_STATE_PAYLOAD;

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
		frm->tx_func(hdr[i]);
	}

	frm->tx_func(hdr_xor);

	cksum_begin(frm->tx_cksum_type, &frm->tx_cksum_scratch);

	return true;
}

/** End frame and enter idle mode */
static void end_frame(SBMP_FrmInst *frm)
{
	if (!frm->tx_enabled) {
		sbmp_error("Can't tx, not enabled.");
		return;
	}

	cksum_end(frm->tx_cksum_type, &frm->tx_cksum_scratch);

	uint32_t cksum = frm->tx_cksum_scratch;

	switch (frm->tx_cksum_type) {
		case SBMP_CKSUM_NONE:
			break; // do nothing

		case SBMP_CKSUM_XOR:
			// 1-byte checksum
			frm->tx_func(cksum & 0xFF);
			break;

		case SBMP_CKSUM_CRC32:
			frm->tx_func(cksum & 0xFF);
			frm->tx_func((cksum >> 8) & 0xFF);
			frm->tx_func((cksum >> 16) & 0xFF);
			frm->tx_func((cksum >> 24) & 0xFF);
	}

	frm->tx_status = FRM_STATE_IDLE; // tx done
}

/** Send a byte in the currently open frame */
bool sbmp_frm_send_byte(SBMP_FrmInst *frm, uint8_t byte)
{
	if (!frm->tx_enabled) {
		sbmp_error("Can't tx, not enabled.");
		return false;
	}

	if (frm->tx_remain == 0 || frm->tx_status != FRM_STATE_PAYLOAD) {
		return false;
	}

	frm->tx_func(byte);
	cksum_update(frm->tx_cksum_type, &frm->tx_cksum_scratch, byte);
	frm->tx_remain--;

	if (frm->tx_remain == 0) {
		end_frame(frm); // checksum & go idle
	}

	return true;
}

/** Send a buffer in the currently open frame */
uint16_t sbmp_frm_send_buffer(SBMP_FrmInst *frm, const uint8_t *buffer, uint16_t length)
{
	if (! frm->tx_enabled) {
		sbmp_error("Can't tx, not enabled.");
		return false;
	}

	if (frm->tx_status != FRM_STATE_PAYLOAD) {
		return false; // invalid call
	}

	if (length == 0) {
		end_frame(frm); // checksum & go idle
		return 0;
	}

	if (frm->tx_remain == 0) {
		return false; // write past EOF (this shouldn't happen)
	}

	uint16_t remain = length;
	while (frm->tx_status == FRM_STATE_PAYLOAD && remain-- > 0) {
		if (! sbmp_frm_send_byte(frm, *buffer++)) {
			remain++; // "push back"
			break;
		}
	}

	return (length - remain);
}

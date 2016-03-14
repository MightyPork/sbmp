#ifndef SBMP_FRAME_H
#define SBMP_FRAME_H

/**
 * SBMP Frame parser.
 *
 * This is the lowest level of the SBMP stack.
 *
 * You can use the frame parser on it's own, if you don't the higher layers.
 * You will still get checksums and other benefits.
 */

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#include "sbmp_config.h"
#include "sbmp_checksum.h"

/**
 * Status returned from the byte rx function.
 */
typedef enum {
	SBMP_RX_OK = 0,   /*!< Byte received */
	SBMP_RX_BUSY,     /*!< SBMP busy with the last packet, try again */
	SBMP_RX_INVALID,  /*!< The byte was rejected - can be ASCII debug, or just garbage. */
	SBMP_RX_DISABLED, /*!< The byte was rejected, because the frame parser is not enabled yet. */
} SBMP_RxStatus;

/** SBMP internal state (context). Allows having multiple SBMP interfaces. */
typedef struct SBMP_FrmInstance_struct SBMP_FrmInst;


/**
 * @brief Initialize the SBMP internal state struct
 *
 * @param state       : Pointer to the state struct to populate. NULL to allocate.
 * @param buffer      : Buffer for Rx. NULL to allocate.
 * @param buffer_size : size of the Rx buffer (or how many bytes to allocate)
 * @param rx_handler  : callback when frame is received
 * @param tx_func     : function for sending bytes (USART Tx)
 * @return pointer to the state struct (passed or allocated)
 */
SBMP_FrmInst *sbmp_frm_init(
		SBMP_FrmInst *frm_or_null,
		uint8_t *buffer_or_null,
		uint16_t buffer_size,
		void (*rx_handler)(uint8_t *payload, uint16_t length, void *user_token),
		void (*tx_func)(uint8_t byte)
);

/**
 * @brief Set the user token value.
 *
 * The token will be passed to the Frame Rx callback.
 * Can be used to identify higher level layer instance.
 *
 * If the token is not set, NULL will be used in the callback.
 *
 * @param state : SBMP state struct
 * @param token : pointer to arbitrary object.
 */
void sbmp_frm_set_user_token(SBMP_FrmInst *frm, void *token);

/**
 * @brief Reset the SBMP frm state, discard partial messages (both rx and tx).
 * @param state : SBMP state struct
 */
void sbmp_frm_reset(SBMP_FrmInst *frm);

/**
 * @brief Enable or disable the frame parser
 * @param state : SBMP state struct
 * @param bool  : true - enable, false - disable
 */
void sbmp_frm_enable(SBMP_FrmInst *frm, bool enable);

/**
 * @brief Handle an incoming byte
 *
 * @param state  : SBMP state struct
 * @param rxbyte : byte received
 * @return status
 */
SBMP_RxStatus sbmp_frm_receive(SBMP_FrmInst *frm, uint8_t rxbyte);

/**
 * @brief Start a frame transmission
 *
 * @param state : SBMP state struct
 * @param cksum_type : checksum to use (0, 32)
 * @param length : payload length
 * @return true if frame was started.
 */
bool sbmp_frm_start(SBMP_FrmInst *frm, SBMP_CksumType cksum_type, uint16_t length);

/**
 * @brief Send one byte in the open frame.
 *
 * If the payload was completed, checksum is be added and
 * the frame is closed.
 *
 * @param state : SBMP state struct
 * @param byte  : byte to send
 * @return true on success (value did fit in a frame)
 */
bool sbmp_frm_send_byte(SBMP_FrmInst *frm, uint8_t byte);

/**
 * @brief Send a data buffer (or a part).
 *
 * If the payload was completed, checksum is be added and
 * the frame is closed.
 *
 * @param state  : SBMP state struct
 * @param buffer : buffer of bytes to send
 * @param length : buffer length (byte count)
 * @return actual sent length (until payload is full)
 */
uint16_t sbmp_frm_send_buffer(SBMP_FrmInst *frm, const uint8_t *buffer, uint16_t length);



// ---- Internal frame struct ------------------------

/** SBMP framing layer Rx / Tx state */
enum SBMP_FrmStatus {
	FRM_STATE_IDLE,         /*!< Ready to start/accept a frame */
	FRM_STATE_CKSUM_TYPE,   /*!< Rx, waiting for checksum type (1 byte) */
	FRM_STATE_LENGTH,       /*!< Rx, waiting for payload length (2 bytes) */
	FRM_STATE_HDRXOR,       /*!< Rx, waiting for header XOR (1 byte) */
	FRM_STATE_PAYLOAD,      /*!< Rx or Tx, payload rx/tx in progress. */
	FRM_STATE_DISCARD,      /*!< Discard rx_length worth of bytes, then end */
	FRM_STATE_CKSUM,        /*!< Rx, waiting for checksum (4 bytes) */
	FRM_STATE_WAIT_HANDLER, /*!< Rx, waiting for rx callback to process the payload */
};

/**
 * Internal state of the SBMP node
 * Placed in the header to allow static allocation.
 */
struct SBMP_FrmInstance_struct {

	// --- reception ---

	uint32_t mb_buf;    /*!< Multi-byte value buffer */
	uint8_t mb_cnt;

	uint8_t rx_hdr_xor; /*!< Header xor scratch field */

	enum SBMP_FrmStatus rx_status;

	bool enabled;       /*!< Frm enabled state. If disabled, all incoming bytes are rejected. */

	uint8_t *rx_buffer;     /*!< Incoming packet buffer */
	uint16_t rx_buffer_i;   /*!< Buffer cursor */
	uint16_t rx_buffer_cap; /*!< Buffer capacity */

	uint16_t rx_length;     /*!< Total payload length */

	SBMP_CksumType rx_cksum_type; /*!< Current packet's checksum type */
	uint32_t rx_cksum_scratch; /*!< crc aggregation field for received data */

	void (*rx_handler)(uint8_t *payload, uint16_t length, void *user_token); /*!< Message received handler */

	void *user_token;    /*!< Arbitrary pointer set by the user. Passed to callbacks.
							  Can be used to identify instance of a higher layer. */

	// --- transmission ---

	uint16_t tx_remain; /*!< Number of remaining bytes to transmit */
	SBMP_CksumType tx_cksum_type;
	uint32_t tx_cksum_scratch; /*!< crc aggregation field for transmit */

	enum SBMP_FrmStatus tx_status;

	// output functions. Only tx_func is needed.
	void (*tx_func)(uint8_t byte);  /*!< Function to send one byte */
};

// ------------------------------------


#endif /* SBMP_FRAME_H */

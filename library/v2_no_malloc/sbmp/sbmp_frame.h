#ifndef SBMP_FRAME_H
#define SBMP_FRAME_H

/**
 * SBMP Framing layer (without malloc).
 */

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#include "sbmp_common.h"

/** Checksum types */
typedef enum {
	SBMP_CKSUM_NONE = 0,   /*!< No checksum */
	SBMP_CKSUM_CRC32 = 32, /*!< ISO CRC-32 */
} SBMP_ChecksumType;

/**
 * Status returned from the byte rx function.
 */
typedef enum {
	SBMP_RX_OK,     /*!< Byte received */
	SBMP_RX_BUSY,   /*!< SBMP busy with the last packet, try again */
	SBMP_RX_INVALID /*!< The byte was rejected - can be ASCII debug, or just garbage. */
} SBMP_RxStatus;

/** SBMP internal state (context). Allows having multiple SBMP interfaces. */
typedef struct SBMP_State_struct SBMP_State;


/**
 * @brief Allocate & initialize the SBMP internal state struct
 *
 * @param frame_handler : frame rx callback.
 * @param buffer_size : size of the payload buffer
 * @return pointer to the allocated struct
 */
void sbmp_init(SBMP_State *state,
		uint8_t *buffer,
		uint16_t buffer_size,
		void (*rx_handler)(uint8_t *payload, uint16_t length),
		void (*tx_func)(uint8_t byte)
);

/**
 * @brief Reset the SBMP state, discard partial messages (both rx and tx).
 * @param state : SBMP state struct
 */
void sbmp_reset(SBMP_State *state);

/**
 * @brief Handle an incoming byte
 *
 * @param state  : SBMP state struct
 * @param rxbyte : byte received
 */
SBMP_RxStatus sbmp_receive(SBMP_State *state, uint8_t rxbyte);

/**
 * @brief Start a frame transmission
 *
 * @param state : SBMP state struct
 * @param cksum_type : checksum to use (0, 32)
 * @param length : payload length
 * @return true if frame was started.
 */
bool sbmp_start_frame(SBMP_State *state, SBMP_ChecksumType cksum_type, uint16_t length);

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
bool sbmp_send_byte(SBMP_State *state, uint8_t byte);

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
uint16_t sbmp_send_buffer(SBMP_State *state, const uint8_t *buffer, uint16_t length);



// ---- Internal frame struct ------------------------

/** SBMP framing layer Rx / Tx state */
enum SBMP_FrameState {
	FRM_STATE_IDLE,         /*!< Ready to start/accept a frame */
	FRM_STATE_CKSUM_TYPE,   /*!< Rx, waiting for checksum type (1 byte) */
	FRM_STATE_LENGTH,       /*!< Rx, waiting for payload length (2 bytes) */
	FRM_STATE_HDRXOR,       /*!< Rx, waiting for header XOR (1 byte) */
	FRM_STATE_PAYLOAD,      /*!< Rx or Tx, payload rx/tx in progress. */
	FRM_STATE_CKSUM,        /*!< Rx, waiting for checksum (4 bytes) */
	FRM_STATE_WAIT_HANDLER, /*!< Rx, waiting for rx callback to process the payload */
};

/**
 * Internal state of the SBMP node
 * Placed in the header to allow static allocation.
 */
struct SBMP_State_struct {

	// --- reception ---

	uint32_t mb_buf; /*!< Multi-byte value buffer */
	uint8_t mb_cnt;

	uint8_t rx_hdr_xor; /*!< Header xor scratch field */

	enum SBMP_FrameState rx_state;

	uint8_t *rx_buffer;   /*!< Incoming packet buffer */
	uint16_t rx_buffer_i;   /*!< Buffer cursor */
	uint16_t rx_buffer_cap; /*!< Buffer capacity */

	uint16_t rx_length; /*!< Total payload length */

	SBMP_ChecksumType rx_cksum_type; /*!< Current packet's checksum type */

	uint32_t rx_crc_scratch; /*!< crc aggregation field for received data */

	void (*rx_handler)(uint8_t *payload, uint16_t length); /*!< Message received handler */

	// --- transmission ---

	uint16_t tx_remain; /*!< Number of remaining bytes to transmit */
	SBMP_ChecksumType tx_cksum_type;
	uint32_t tx_crc_scratch; /*!< crc aggregation field for transmit */
	enum SBMP_FrameState tx_state;

	// output functions. Only tx_func is needed.
	void (*tx_func)(uint8_t byte);  /*!< Function to send one byte */
	void (*tx_lock_func)(void);     /*!< Lock the serial interface tx (called before a frame) */
	void (*tx_release_func)(void);  /*!< Release the serial interface tx (called after a frame) */
};

// ------------------------------------


#endif /* SBMP_FRAME_H */

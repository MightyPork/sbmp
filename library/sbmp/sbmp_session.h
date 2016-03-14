#ifndef SBMP_SESSION_H
#define SBMP_SESSION_H

/**
 * SBMP session layer
 *
 * This layer provides an abstraction over all of SBMP.
 *
 * Start by creating "endpoint" with sbmp_ep_init(), then configure and enable it.
 *
 * Next you can trigger a handshake, which assigns your endpoint the origin bit,
 * and obtains information about your peer (it's buffer size and preferred checksum).
 *
 * You can still interact with the framing layer directly, but it shouldn't be needed.
 */

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

#include "sbmp_config.h"
#include "sbmp_datagram.h"
#include "sbmp_frame.h"

typedef enum {
	SBMP_HSK_NOT_STARTED = 0,     /*!< Initial state, unconfigured */
	SBMP_HSK_SUCCESS = 1,         /*!< Handshake done, origin assigned. */
	SBMP_HSK_AWAIT_REPLY = 2,     /*!< Request sent, awaiting a reply */
	SBMP_HSK_CONFLICT = 3,        /*!< Conflict occured during HSK */
} SBMP_HandshakeState;


/** SBMP Endpoint (session)  structure */
typedef struct {
	bool origin;                            /*!< Local origin bit */
	uint16_t next_session;                  /*!< Next session number */

	void (*rx_handler)(SBMP_Datagram *dg);  /*!< Datagram receive handler */

	SBMP_FrmState frm_state;                /*!< Framing layer internal state */

	// Handshake
	SBMP_HandshakeState hsk_state;     /*!< Handshake progress */
	uint16_t hsk_session;                   /*!< Session number of the handshake request message */
	uint16_t peer_buffer_size;              /*!< Peer's buffer size (obtained during handshake) */
	SBMP_ChecksumType peer_preferred_cksum; /*!< Peer's preferred checksum type */

	// Our info for the peer
	uint16_t buffer_size;                   /*!< Our buffer size */
	SBMP_ChecksumType preferred_cksum;      /*!< Our preferred checksum */
} SBMP_Endpoint;


/**
 * @brief Initialize the endpoint.
 *
 * @param ep          : Endpoint struct pointer, or NULL to allocate one.
 * @param buffer      : Rx buffer. NULL to allocate one.
 * @param buffer_size : Rx buffer length
 * @param dg_rx_handler : Datagram received handler - the argument structure is valid ONLY within the function!
 * @param tx_func     : Function to send a byte to USART
 * @return the endpoint struct pointer (allocated if ep was NULL)
 */
SBMP_Endpoint *sbmp_ep_init(SBMP_Endpoint *ep,
		uint8_t *buffer,
		uint16_t buffer_size,
		void (*dg_rx_handler)(SBMP_Datagram *dg),
		void (*tx_func)(uint8_t byte)
);

/** Set session number (good to randomize before starting a handshake) */
void sbmp_ep_seed_session(SBMP_Endpoint *ep, uint16_t sesn);

/**
 * Set the origin bit.
 * This can be used instead of handshake if origins are pre-assigned.
 */
void sbmp_ep_set_origin(SBMP_Endpoint *endp, bool bit);

/** Set the preferred checksum for this peer. */
void sbmp_ep_set_preferred_cksum(SBMP_Endpoint *endp, SBMP_ChecksumType cksum_type);

/**
 * @brief Start a message as a reply, with CRC32
 *
 * @param ep         : Endpoint struct
 * @param type       : Datagram type ID
 * @param length     : Datagram payload length (bytes)
 * @param sesn       : Session number of message this is a reply to.
 * @return success
 */
bool sbmp_ep_start_response(SBMP_Endpoint *ep, SBMP_DgType type, uint16_t length, uint16_t sesn);

/**
 * @brief Start a message in a new session, with CRC32
 *
 * @param ep         : Endpoint struct
 * @param type       : Datagram type ID
 * @param length     : Datagram payload length (bytes)
 * @param sesn       : Var to store session number, NULL = don't store.
 * @return success
 */
bool sbmp_ep_start_session(SBMP_Endpoint *ep, SBMP_DgType type, uint16_t length, uint16_t *sesn_ptr);

/**
 * @brief Send one byte in the current message
 *
 * @param ep    : Endpoint struct
 * @param byte  : byte to send
 * @return true on success
 */
bool sbmp_ep_send_byte(SBMP_Endpoint *ep, uint8_t byte);

/**
 * @brief Send a data buffer (or a part) in the current message
 *
 * @param ep     : Endpoint struct
 * @param buffer : buffer of bytes to send
 * @param length : buffer length (byte count)
 * @return actual sent length (until payload is full)
 */
uint16_t sbmp_ep_send_buffer(SBMP_Endpoint *ep, const uint8_t *buffer, uint16_t length);

/**
 * @brief Send a message in a new session.
 *
 * @param ep         : Endpoint struct
 * @param type       : Datagram type ID
 * @param buffer     : Buffer with data to send
 * @param length     : Datagram payload length (bytes)
 * @param sesn_ptr       : Session number
 * @param sent_bytes_ptr : Var to store NR of sent bytes. NULL = don't store.
 * @return success
 */
bool sbmp_ep_send_response(
		SBMP_Endpoint *ep,
		SBMP_DgType type,
		const uint8_t *buffer,
		uint16_t length,
		uint16_t sesn_ptr,
		uint16_t *sent_bytes_ptr);

/**
 * @brief Send a message in a new session.
 *
 * @param ep     : Endpoint struct
 * @param type   : Datagram type ID
 * @param buffer : Buffer with data to send
 * @param length : Datagram payload length (bytes)
 * @param sesn_ptr       : Var to store session number. NULL = don't store.
 * @param sent_bytes_ptr : Var to store NR of sent bytes. NULL = don't store.
 * @return success
 */
bool sbmp_ep_send_message(
		SBMP_Endpoint *ep,
		SBMP_DgType type,
		const uint8_t *buffer,
		uint16_t length,
		uint16_t *sesn,
		uint16_t *sent_bytes);

/**
 * @brief Start a handshake (origin bit arbitration)
 * @param ep : Endpoint struct
 */
bool sbmp_ep_start_handshake(SBMP_Endpoint *ep);

/**
 * @brief Receive a byte from USART (is passed to the framing layer)
 * @param ep   : Endpoint struct
 * @param byte : Byte received from USART
 * @return true if byte was consumed
 */
SBMP_RxStatus sbmp_ep_receive(SBMP_Endpoint *ep, uint8_t byte);

/** Get current handshake state */
SBMP_HandshakeState sbmp_ep_handshake_status(SBMP_Endpoint *ep);

/** Enable or disable the framing layer backing this EP */
void sbmp_ep_enable(SBMP_Endpoint *ep, bool enable);

#endif /* SBMP_SESSION_H */

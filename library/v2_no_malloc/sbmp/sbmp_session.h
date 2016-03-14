#ifndef SBMP_SESSION_H
#define SBMP_SESSION_H

#include <stdint.h>
#include <stdbool.h>
#include <malloc.h>

#include "sbmp_frame.h"
#include "sbmp_datagram.h"

/** SBMP Endpoint (session)  structure */
typedef struct {
	bool origin;           /*!< Local origin bit */
	uint16_t next_session; /*!< Next session number */

	void (*rx_handler)(SBMP_Datagram *dg); /*!< Datagram receive handler */

	SBMP_FrmState frm_state;  /*!< Framing layer internal state */
} SBMP_Endpoint;


/**
 * @brief Initialize the endpoint.
 *
 * @param ep          : Endpoint var pointer, or NULL to allocate one.
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

/** Set the origin bit */
static inline
void sbmp_ep_set_origin(SBMP_Endpoint *endp, bool bit)
{
	endp->origin = bit;
}

/** Get a new session number */
uint16_t sbmp_ep_new_session(SBMP_Endpoint *ep);

/**
 * @brief Start a message as a reply, with CRC32
 *
 * @param ep         : Endpoint state
 * @param type       : Datagram type ID
 * @param length     : Datagram payload length (bytes)
 * @param sesn       : Session number of message this is a reply to.
 * @return success
 */
static inline
bool sbmp_ep_start_response(SBMP_Endpoint *ep, SBMP_DgType type, uint16_t length, uint16_t sesn)
{
	return sbmp_start_datagram(&ep->frm_state, 32, sesn, type, length);
}

/**
 * @brief Start a message in a new session, with CRC32
 *
 * @param ep         : Endpoint state
 * @param type       : Datagram type ID
 * @param length     : Datagram payload length (bytes)
 * @return success
 */
static inline
bool sbmp_ep_start_session(SBMP_Endpoint *ep, SBMP_DgType type, uint16_t length)
{
	return sbmp_ep_start_response(ep, type, length, sbmp_ep_new_session(ep));
}

/**
 * @brief Send one byte in the current message
 *
 * @param ep    : Endpoint state
 * @param byte  : byte to send
 * @return true on success
 */
static inline bool sbmp_ep_send_byte(SBMP_Endpoint *ep, uint8_t byte)
{
	return sbmp_send_byte(&ep->frm_state, byte);
}

/**
 * @brief Send a data buffer (or a part) in the current message
 *
 * @param ep     : Endpoint state
 * @param buffer : buffer of bytes to send
 * @param length : buffer length (byte count)
 * @return actual sent length (until payload is full)
 */
static inline
uint16_t sbmp_ep_send_buffer(SBMP_Endpoint *ep, const uint8_t *buffer, uint16_t length)
{
	return sbmp_send_buffer(&ep->frm_state, buffer, length);
}

/**
 * @brief Send a message in a new session.
 *
 * @param ep         : Endpoint state
 * @param type       : Datagram type ID
 * @param buffer     : Buffer with data to send
 * @param length     : Datagram payload length (bytes)
 * @return number of really sent bytes
 */
uint16_t sbmp_ep_send_response(SBMP_Endpoint *ep, SBMP_DgType type, const uint8_t *buffer, uint16_t length, uint16_t sesn);

/**
 * @brief Send a message in a new session.
 *
 * @param ep         : Endpoint state
 * @param type       : Datagram type ID
 * @param buffer     : Buffer with data to send
 * @param length     : Datagram payload length (bytes)
 * @return number of really sent bytes
 */
static inline
uint16_t sbmp_ep_send_message(SBMP_Endpoint *ep, SBMP_DgType type, const uint8_t *buffer, uint16_t length)
{
	return sbmp_ep_send_response(ep, type, buffer, length, sbmp_ep_new_session(ep));
}

#endif /* SBMP_SESSION_H */

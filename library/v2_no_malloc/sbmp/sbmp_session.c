#include "sbmp_session.h"

static void rx_handler(uint8_t *buf, uint16_t len, void *token)
{
	// endpoint pointer is stored in the user token
	SBMP_Endpoint *ep = (SBMP_Endpoint *)token;

	SBMP_Datagram dg;
	ep->rx_handler(sbmp_parse_datagram(&dg, buf, len));
}

/**
 * @brief Initialize the endpoint.
 *
 * @param ep          : Endpoint var pointer, or NULL to allocate one.
 * @param buffer      : Rx buffer. NULL to allocate one.
 * @param buffer_size : Rx buffer length
 * @return the endpoint struct pointer (allocated if ep was NULL)
 */
SBMP_Endpoint *sbmp_ep_init(SBMP_Endpoint *ep,
		uint8_t *buffer,
		uint16_t buffer_size,
		void (*dg_rx_handler)(SBMP_Datagram *dg),
		void (*tx_func)(uint8_t byte)
) {
	if (ep == NULL) {
		// request to allocate it
		ep = malloc(sizeof(SBMP_Endpoint));
	}

	// set up the framing layer
	sbmp_frm_init(&ep->frm_state, buffer, buffer_size, rx_handler, tx_func);

	ep->next_session = 0;
	ep->origin = 0;
	ep->rx_handler = dg_rx_handler;

	return ep;
}

/** Get a new session number */
uint16_t sbmp_ep_new_session(SBMP_Endpoint *ep)
{
	uint16_t sesn = ep->next_session;

	if (++ep->next_session == 0x8000) {
		// overflow into the origin bit
		ep->next_session = 0; // start from zero
	}

	return sesn | (uint16_t)(ep->origin << 15); // add the origin bit
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
uint16_t sbmp_ep_send_response(SBMP_Endpoint *ep, SBMP_DgType type, const uint8_t *buffer, uint16_t length, uint16_t sesn)
{
	bool suc;

	suc = sbmp_ep_start_response(ep, type, length, sesn);
	if (!suc) return 0; // unlikely

	return sbmp_ep_send_buffer(ep, buffer, length);
}

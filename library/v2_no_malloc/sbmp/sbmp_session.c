#include "sbmp_session.h"
#include <inttypes.h>

// protos
static void handle_hsk_datagram(SBMP_Endpoint *ep, SBMP_Datagram *dg);


static void ep_rx_handler(uint8_t *buf, uint16_t len, void *token)
{
	// endpoint pointer is stored in the user token
	SBMP_Endpoint *ep = (SBMP_Endpoint *)token;

	SBMP_Datagram dg;
	if (NULL != sbmp_parse_datagram(&dg, buf, len)) {
		// payload parsed OK

		// check if handshake datagram, else call user callback.
		handle_hsk_datagram(ep, &dg);
	}
}

/**
 * @brief Initialize the endpoint.
 *
 * @param ep          : Endpoint var pointer, or NULL to allocate one.
 * @param buffer      : Rx buffer. NULL to allocate one.
 * @param buffer_size : Rx buffer length
 * @return the endpoint struct pointer (allocated if ep was NULL)
 */
SBMP_Endpoint *sbmp_ep_init(
		SBMP_Endpoint *ep,
		uint8_t *buffer,
		uint16_t buffer_size,
		void (*dg_rx_handler)(SBMP_Datagram *dg),
		void (*tx_func)(uint8_t byte))
{
	if (ep == NULL) {
		// request to allocate it
		ep = malloc(sizeof(SBMP_Endpoint));
	}

	// set up the framing layer
	sbmp_frm_init(&ep->frm_state, buffer, buffer_size, ep_rx_handler, tx_func);

	ep->next_session = 0;
	ep->origin = 0;
	ep->rx_handler = dg_rx_handler;

	// init the handshake status
	ep->hsk_session = 0;
	ep->hsk_state = SBMP_HSK_NOT_STARTED;
	ep->peer_buffer_size = 0xFFFF; // max possible buffer
	ep->peer_preferred_cksum = SBMP_CKSUM_CRC32;
	// our info for the peer
	ep->buffer_size = buffer_size;
	ep->preferred_cksum = SBMP_CKSUM_CRC32;

	return ep;
}


// --- Customizing settings ---

/** Set session number (good to randomize before first message) */
void sbmp_ep_seed_session(SBMP_Endpoint *ep, uint16_t sesn)
{
	ep->next_session = sesn & 0x7FFF;
}

/** Set the origin bit (bypass handshake) */
void sbmp_ep_set_origin(SBMP_Endpoint *endp, bool bit)
{
	endp->origin = bit;
}

/** Set the preferred checksum. */
void sbmp_ep_set_preferred_cksum(SBMP_Endpoint *endp, SBMP_ChecksumType cksum_type)
{
	endp->preferred_cksum = cksum_type;
}

// ---

/** Get a new session number */
static uint16_t next_session(SBMP_Endpoint *ep)
{
	uint16_t sesn = ep->next_session;

	if (++ep->next_session == 0x8000) {
		// overflow into the origin bit
		ep->next_session = 0; // start from zero
	}

	return sesn | (uint16_t)(ep->origin << 15); // add the origin bit
}


// --- Header/body send funcs ---

/** Start a message as a reply, with CRC32 */
bool sbmp_ep_start_response(SBMP_Endpoint *ep, SBMP_DgType type, uint16_t length, uint16_t sesn)
{
	return sbmp_start_datagram(&ep->frm_state, 32, sesn, type, length);
}

/** Start a message in a new session, with CRC32 */
bool sbmp_ep_start_session(SBMP_Endpoint *ep, SBMP_DgType type, uint16_t length, uint16_t *sesn_ptr)
{
	uint16_t sn = next_session(ep);

	bool suc = sbmp_ep_start_response(ep, type, length, sn);
	if (suc) {
		if (sesn_ptr != NULL) *sesn_ptr = sn;
	}

	return suc;
}

/** Send one byte in the current message */
bool sbmp_ep_send_byte(SBMP_Endpoint *ep, uint8_t byte)
{
	return sbmp_send_byte(&ep->frm_state, byte);
}

/** Send a data buffer (or a part) in the current message */
uint16_t sbmp_ep_send_buffer(SBMP_Endpoint *ep, const uint8_t *buffer, uint16_t length)
{
	return sbmp_send_buffer(&ep->frm_state, buffer, length);
}


// --- All-in-one send funcs ---

/** Send a message in a session. */
bool sbmp_ep_send_response(
		SBMP_Endpoint *ep,
		SBMP_DgType type,
		const uint8_t *buffer,
		uint16_t length,
		uint16_t sesn,
		uint16_t *sent_bytes_ptr)
{
	bool suc = sbmp_ep_start_response(ep, type, length, sesn);

	if (suc) {
		uint16_t sent = sbmp_ep_send_buffer(ep, buffer, length);

		if (sent_bytes_ptr != NULL) *sent_bytes_ptr = sent;
	}

	return suc;
}

/** Send message in a new session */
bool sbmp_ep_send_message(
		SBMP_Endpoint *ep,
		SBMP_DgType type,
		const uint8_t *buffer,
		uint16_t length,
		uint16_t *sesn_ptr,
		uint16_t *sent_bytes_ptr)
{
	uint16_t sn = next_session(ep);

	bool suc = sbmp_ep_send_response(ep, type, buffer, length, sn, sent_bytes_ptr);

	if (suc) {
		if (sesn_ptr != NULL) *sesn_ptr = sn;
	}

	return suc;
}


// --- Handshake ---

#define HSK_PAYLOAD_LEN 3
/** Prepare a buffer to send to peer during handshake */
static void populate_hsk_buf(SBMP_Endpoint *ep, uint8_t* buf)
{
	// [ pref_crc 1B | buf_size 2B ]

	buf[0] = ep->preferred_cksum;
	buf[1] = U16_LSB(ep->buffer_size);
	buf[2] = U16_MSB(ep->buffer_size);
}

/** Parse peer info from received handhsake dg payload */
void parse_peer_hsk_buf(SBMP_Endpoint *ep, const uint8_t* buf)
{
	ep->peer_preferred_cksum = buf[0];
	ep->peer_buffer_size = (uint16_t)(buf[1] | (buf[2] << 8));

	sbmp_info("HSK success, peer buf %"PRIu16", pref cksum %d",
			  ep->peer_buffer_size,
			  ep->peer_preferred_cksum);
}

/**
 * @brief Start a handshake (origin bit arbitration)
 * @param ep : Endpoint state
 */
bool sbmp_ep_start_handshake(SBMP_Endpoint *ep)
{
	if (ep->hsk_state == SBMP_HSK_AWAIT_REPLY) {
		// busy now
		sbmp_error("Can't start HSK, in progress already.");
		return false;
	}

	ep->hsk_state = 0;

	uint8_t buf[HSK_PAYLOAD_LEN];
	populate_hsk_buf(ep, buf);

	bool suc = sbmp_ep_send_message(ep, SBMP_DG_HSK_START, buf, 3, &ep->hsk_session, NULL);

	if (suc) {
		ep->hsk_state = SBMP_HSK_AWAIT_REPLY;
	}

	return suc;
}

static void handle_hsk_datagram(SBMP_Endpoint *ep, SBMP_Datagram *dg)
{
	bool hsk_start = (dg->type == SBMP_DG_HSK_START);
	bool hsk_accept = (dg->type == SBMP_DG_HSK_ACCEPT);
	bool hsk_conflict = (dg->type == SBMP_DG_HSK_CONFLICT);

	if (hsk_start || hsk_accept || hsk_conflict) {
		// prepare payload to send in response
		uint8_t our_info_pld[HSK_PAYLOAD_LEN];
		populate_hsk_buf(ep, our_info_pld);

		if (hsk_start) {
			// peer requests origin
			sbmp_info("Rx HSK request");

			if (ep->hsk_state == SBMP_HSK_AWAIT_REPLY) {
				// conflict occured - we're already waiting for a reply.
				sbmp_ep_send_response(ep, SBMP_DG_HSK_CONFLICT, our_info_pld, HSK_PAYLOAD_LEN, dg->session, NULL);
				ep->hsk_state = SBMP_HSK_CONFLICT;

				sbmp_error("HSK conflict");
			} else {
				// we're idle, accept the request.
				bool peer_origin = (dg->session & 0x8000) >> 15;
				sbmp_ep_set_origin(ep, !peer_origin);

				// read peer's info
				if (dg->length >= HSK_PAYLOAD_LEN) {
					parse_peer_hsk_buf(ep, dg->payload);
				}

				ep->hsk_state = SBMP_HSK_SUCCESS;

				// Send Accept response
				sbmp_ep_send_response(ep, SBMP_DG_HSK_ACCEPT, our_info_pld, HSK_PAYLOAD_LEN, dg->session, NULL);
			}
		} else if (hsk_accept) {
			// peer accepted our request
			sbmp_info("Rx HSK accept");

			if (ep->hsk_state != SBMP_HSK_AWAIT_REPLY || ep->hsk_session != dg->session) {
				// but we didn't send any request
				sbmp_error("Rx unexpected HSK accept, ignoring.");
			} else {
				// OK, we were waiting for this reply

				// read peer's info
				if (dg->length >= HSK_PAYLOAD_LEN) {
					parse_peer_hsk_buf(ep, dg->payload);
				}

				ep->hsk_state = SBMP_HSK_SUCCESS;
			}
		} else if (hsk_conflict) {
			// peer rejected our request due to conflict
			sbmp_info("Rx HSK conflict");

			if (ep->hsk_state != SBMP_HSK_AWAIT_REPLY || ep->hsk_session != dg->session) {
				// but we didn't send any request
				sbmp_error("Rx unexpected HSK conflict, ignoring.");
			} else {
				// Acknowledge the conflict

				// reset everything
				sbmp_frm_reset(&ep->frm_state);

				ep->hsk_state = SBMP_HSK_CONFLICT;
			}
		}

	} else {
		// Not a HSK message
		ep->rx_handler(dg);
	}
}

#include <inttypes.h>

#include "sbmp_config.h"
#include "sbmp_logging.h"
#include "sbmp_session.h"

// protos
static void handle_hsk_datagram(SBMP_Endpoint *ep, SBMP_Datagram *dg);

// lsb, msb for uint16_t
#define U16_LSB(x) ((x) & 0xFF)
#define U16_MSB(x) ((x >> 8) & 0xFF)

// length of the payload sent with a handshake packet.
#define HSK_PAYLOAD_LEN 3
// Datagram header length - 2 B sesn, 1 B type
#define DATAGRA_HEADER_LEN 3


/** Rx handler that is assigned to the framing layer */
static void ep_rx_handler(uint8_t *buf, uint16_t len, void *token)
{
	// endpoint pointer is stored in the user token
	SBMP_Endpoint *ep = (SBMP_Endpoint *)token;

	if (NULL != sbmp_dg_parse(&ep->static_dg, buf, len)) {
		// payload parsed OK

		// check if handshake datagram, else call user callback.
		handle_hsk_datagram(ep, &ep->static_dg);
	}
}

/**
 * @brief Initialize the endpoint.
 *
 * @param ep          : Endpoint var pointer, or NULL to allocate one.
 * @param buffer      : Rx buffer. NULL to allocate one.
 * @param buffer_size : Rx buffer length
 * @param listener_slots      : session listener slots (for multi-message sessions), NULL to malloc.
 * @param listener_slot_count : number of slots in the array (or to allocate)
 * @return the endpoint struct pointer (allocated if ep was NULL)
 */
SBMP_Endpoint *sbmp_ep_init(
		// endpoint struct, NULL to malloc.
		SBMP_Endpoint *ep,
		// payload buffer, NULL to malloc.
		uint8_t *buffer, uint16_t buffer_size,
		// receive handler
		void (*dg_rx_handler)(SBMP_Datagram *dg),
		// byte transmit function for the framing layer
		void (*tx_func)(uint8_t byte))
{
	// indicate that the obj was malloc'd here, and should be freed on error
	bool ep_mallocd = false;

	if (ep == NULL) {
		// request to allocate it
		#if SBMP_MALLOC
			ep = malloc(sizeof(SBMP_Endpoint));
			if (!ep) return NULL; // malloc failed
			ep_mallocd = true;
		#else
			return NULL; // fail
		#endif
	}

	// set the listener fields
	ep->listeners = NULL;
	ep->listener_count = 0;

	// set up the framing layer
	SBMP_FrmInst *alloc_frm = sbmp_frm_init(&ep->frm, buffer, buffer_size, ep_rx_handler, tx_func);
	if (!alloc_frm) {
		// the buffer or frm allocation failed
		if (ep_mallocd) free(ep);
		return NULL;
	}

	// set token, so callback knows what EP it's for.
	sbmp_frm_set_user_token(&ep->frm, (void *) ep);

	ep->rx_handler = dg_rx_handler;
	ep->buffer_size = buffer_size; // sent to the peer

#if SBMP_HAS_CRC32
	ep->peer_pref_cksum = SBMP_CKSUM_CRC32;
	ep->pref_cksum = SBMP_CKSUM_CRC32;
#else
	ep->peer_pref_cksum = SBMP_CKSUM_XOR;
	ep->pref_cksum = SBMP_CKSUM_XOR;
#endif

	// reset state information
	sbmp_ep_reset(ep);

	return ep;
}


bool sbmp_ep_init_listeners(SBMP_Endpoint *ep, SBMP_SessionListenerSlot *listener_slots, uint16_t slot_count)
{
	// NULL is allowed only if count is 0
	if (listener_slots == NULL && slot_count > 0) {
		// request to allocate it
		#if SBMP_MALLOC
			listener_slots = malloc(sizeof(SBMP_SessionListenerSlot) * slot_count);
			if (!listener_slots) { // malloc failed
				return false;
			}
		#else
			return false;
		#endif
	}

	// set the listener fields
	ep->listeners = listener_slots;
	ep->listener_count = slot_count;

	return true;
}

/**
 * @brief Reset an endpoint and it's Framing Layer
 *
 * This discards all state information.
 *
 * @param ep : Endpoint
 */
void sbmp_ep_reset(SBMP_Endpoint *ep)
{
	ep->next_session = 0;
	ep->origin = 0;

	// init the handshake status
	ep->hsk_session = 0;
	ep->hsk_status = SBMP_HSK_NOT_STARTED;

	ep->peer_buffer_size = 0xFFFF; // max possible buffer

	sbmp_frm_reset(&ep->frm);
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
void sbmp_ep_set_preferred_cksum(SBMP_Endpoint *endp, SBMP_CksumType cksum_type)
{
	if (cksum_type == SBMP_CKSUM_CRC32 && !SBMP_HAS_CRC32) {
		sbmp_error("CRC32 not avail, using XOR instead.");
		cksum_type = SBMP_CKSUM_XOR;
	}

	endp->pref_cksum = cksum_type;
}


/** Enable or disable RX in the FrmInst backing this Endpoint */
void sbmp_ep_enable_rx(SBMP_Endpoint *ep, bool enable_rx)
{
	sbmp_frm_enable_rx(&ep->frm, enable_rx);
}

/** Enable or disable TX in the FrmInst backing this Endpoint */
void sbmp_ep_enable_tx(SBMP_Endpoint *ep, bool enable_tx)
{
	sbmp_frm_enable_tx(&ep->frm, enable_tx);
}

/** Enable or disable Rx & TX in the FrmInst backing this Endpoint */
void sbmp_ep_enable(SBMP_Endpoint *ep, bool enable)
{
	sbmp_frm_enable(&ep->frm, enable);
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

/** Start a message as a reply */
bool sbmp_ep_start_response(SBMP_Endpoint *ep, SBMP_DgType type, uint16_t length, uint16_t sesn)
{
	uint16_t peer_accepts = ep->peer_buffer_size - DATAGRA_HEADER_LEN;

	if (length > peer_accepts) {
		sbmp_error("Msg too long (%"PRIu16" B), peer accepts max %"PRIu16" B.", length, peer_accepts);
		return false;
	}

	return sbmp_dg_start(&ep->frm, ep->peer_pref_cksum, sesn, type, length);
}

/** Start a message in a new session */
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
	return sbmp_frm_send_byte(&ep->frm, byte);
}

/** Send a data buffer (or a part) in the current message */
uint16_t sbmp_ep_send_buffer(SBMP_Endpoint *ep, const uint8_t *buffer, uint16_t length)
{
	return sbmp_frm_send_buffer(&ep->frm, buffer, length);
}

/** Rx, pass to framing layer */
SBMP_RxStatus sbmp_ep_receive(SBMP_Endpoint *ep, uint8_t byte)
{
	return sbmp_frm_receive(&ep->frm, byte);
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
	// This juggling with session nr is because it wouldn't work
	// without actual hardware delay otherwise.

	uint16_t sn = next_session(ep);

	uint16_t old_sesn = 0;
	if (sesn_ptr != NULL) {
		old_sesn = *sesn_ptr;
		*sesn_ptr = sn;
	}

	bool suc = sbmp_ep_send_response(ep, type, buffer, length, sn, sent_bytes_ptr);

	if (!suc) {
		if (sesn_ptr != NULL) {
			*sesn_ptr = old_sesn; // restore
		}
	}

	return suc;
}


// --- Handshake ---
/**
 * Prepare a buffer to send to peer during handshake
 *
 * The buffer is long HSK_PAYLOAD_LEN bytes
 */
static void populate_hsk_buf(SBMP_Endpoint *ep, uint8_t* buf)
{
	// [ pref_crc 1B | buf_size 2B ]

	buf[0] = ep->pref_cksum;
	buf[1] = U16_LSB(ep->buffer_size);
	buf[2] = U16_MSB(ep->buffer_size);
}

/** Parse peer info from received handhsake dg payload */
static void parse_peer_hsk_buf(SBMP_Endpoint *ep, const uint8_t* buf)
{
	ep->peer_pref_cksum = buf[0];
	ep->peer_buffer_size = (uint16_t)(buf[1] | (buf[2] << 8));

	sbmp_info("HSK success, peer buf %"PRIu16", pref cksum %d",
			  ep->peer_buffer_size,
			  ep->peer_pref_cksum);

	// check if checksum available
	if (ep->peer_pref_cksum == SBMP_CKSUM_CRC32 && !SBMP_HAS_CRC32) {
		sbmp_error("CRC32 not avail, using XOR as peer's pref cksum.");
		ep->peer_pref_cksum = SBMP_CKSUM_XOR;
	}
}

/**
 * @brief Start a handshake (origin bit arbitration)
 * @param ep : Endpoint state
 */
bool sbmp_ep_start_handshake(SBMP_Endpoint *ep)
{
	sbmp_ep_abort_handshake(ep);

	uint8_t buf[HSK_PAYLOAD_LEN];
	populate_hsk_buf(ep, buf);

	ep->hsk_status = SBMP_HSK_AWAIT_REPLY;

	bool suc = sbmp_ep_send_message(ep, SBMP_DG_HSK_START, buf, 3, &ep->hsk_session, NULL);

	if (!suc) {
		ep->hsk_status = SBMP_HSK_NOT_STARTED;
	}

	return suc;
}

/** Get hsk state */
SBMP_HandshakeStatus sbmp_ep_handshake_status(SBMP_Endpoint *ep)
{
	return ep->hsk_status;
}

/** Abort current handshake & discard hsk session */
void sbmp_ep_abort_handshake(SBMP_Endpoint *ep)
{
	ep->hsk_session = 0;
	ep->hsk_status = SBMP_HSK_NOT_STARTED;
}

/**
 * @brief Process handshake datagrams & update handshake state accordingly.
 *
 * Non-handshake datagrams are passed on to the user Rx callback.
 *
 * @param ep : endpoint
 * @param dg : datagram
 */
static void handle_hsk_datagram(SBMP_Endpoint *ep, SBMP_Datagram *dg)
{
	bool hsk_start = (dg->type == SBMP_DG_HSK_START);
	bool hsk_accept = (dg->type == SBMP_DG_HSK_ACCEPT);
	bool hsk_conflict = (dg->type == SBMP_DG_HSK_CONFLICT);

	if (hsk_start || hsk_accept || hsk_conflict) {
		// prepare payload to send in response
		uint8_t our_info_pld[HSK_PAYLOAD_LEN];
		populate_hsk_buf(ep, our_info_pld);

		//printf("hsk_state = %d, hsk_ses %d, dg_ses %d\n",ep->hsk_state,  ep->hsk_session, dg->session);

		if (hsk_start) {
			// peer requests origin
			sbmp_info("Rx HSK request");

			if (ep->hsk_status == SBMP_HSK_AWAIT_REPLY) {
				// conflict occured - we're already waiting for a reply.
				sbmp_ep_send_response(ep, SBMP_DG_HSK_CONFLICT, our_info_pld, HSK_PAYLOAD_LEN, dg->session, NULL);
				ep->hsk_status = SBMP_HSK_CONFLICT;

				sbmp_error("HSK conflict");
			} else {
				// we're idle, accept the request.
				bool peer_origin = (dg->session & 0x8000) >> 15;
				sbmp_ep_set_origin(ep, !peer_origin);

				// read peer's info
				if (dg->length >= HSK_PAYLOAD_LEN) {
					parse_peer_hsk_buf(ep, dg->payload);
				}

				ep->hsk_status = SBMP_HSK_SUCCESS;

				// Send Accept response
				sbmp_ep_send_response(ep, SBMP_DG_HSK_ACCEPT, our_info_pld, HSK_PAYLOAD_LEN, dg->session, NULL);
			}
		} else if (hsk_accept) {
			// peer accepted our request
			sbmp_info("Rx HSK accept");

			if (ep->hsk_status != SBMP_HSK_AWAIT_REPLY || ep->hsk_session != dg->session) {
				// but we didn't send any request
				sbmp_error("Rx unexpected HSK accept, ignoring.");

			} else {
				// OK, we were waiting for this reply

				// read peer's info
				if (dg->length >= HSK_PAYLOAD_LEN) {
					parse_peer_hsk_buf(ep, dg->payload);
				}

				ep->hsk_status = SBMP_HSK_SUCCESS;
			}
		} else if (hsk_conflict) {
			// peer rejected our request due to conflict
			sbmp_info("Rx HSK conflict");

			if (ep->hsk_status != SBMP_HSK_AWAIT_REPLY || ep->hsk_session != dg->session) {
				// but we didn't send any request
				sbmp_error("Rx unexpected HSK conflict, ignoring.");
			} else {
				// Acknowledge the conflict

				// reset everything
				sbmp_frm_reset(&ep->frm);

				ep->hsk_status = SBMP_HSK_CONFLICT;
			}
		}

	} else {
		// Not a HSK message
		ep->rx_handler(dg);
	}
}


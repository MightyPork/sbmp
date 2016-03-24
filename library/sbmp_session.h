#ifndef SBMP_SESSION_H
#define SBMP_SESSION_H

/**
 * SBMP session layer
 *
 * This layer provides an abstraction over all of SBMP.
 *
 * Start by creating "endpoint" with sbmp_ep_init(), then configure and enable it.
 *
 * Next you should trigger a handshake, which assigns your endpoint the origin bit,
 * and obtains information about your peer (it's buffer size and preferred checksum).
 *
 * You can still interact with the framing layer directly, but it shouldn't be needed.
 */

#include "esp8266.h"

#include "sbmp_config.h"
#include "sbmp_datagram.h"
#include "sbmp_frame.h"
#include "payload_parser.h"

/**
 * Handshake status
 */
typedef enum {
	SBMP_HSK_IDLE = 0,            /*!< Initial state, origin unconfigured. Idle. */
	SBMP_HSK_SUCCESS = 1,         /*!< Handshake done, origin assigned. Idle. */
	SBMP_HSK_AWAIT_REPLY = 2,     /*!< Request sent, awaiting a reply */
	SBMP_HSK_CONFLICT = 3,        /*!< Conflict occured during HSK */
} SBMP_HandshakeStatus;


/** Forward declaration of the endpoint struct */
typedef struct SBMP_Endpoint_struct SBMP_Endpoint;

/**
 * Session listener function.
 *
 * The obj var can be used to preseve state between calls, each listener has it's own.
 */
typedef void (*SBMP_SessionListener)(SBMP_Endpoint *ep, SBMP_Datagram *dg, void **obj);

/**
 * Session listener slot.
 *
 * Used internally when session listener is installed,
 * declared in the header to allow static allocation.
 */
typedef struct {
	uint16_t session;              /*!< The session this slot is listening to */
	SBMP_SessionListener callback; /*!< Message handler func, null = the slot is unused */
	void *obj;                     /*!< Opaque pointer to user data that can be used inside the listener. */
} SBMP_SessionListenerSlot;


/** SBMP Endpoint (session) structure */
struct SBMP_Endpoint_struct {
	bool origin;                     /*!< Local origin bit */
	uint16_t next_session;           /*!< Next session number */

	SBMP_SessionListenerSlot *listeners; /*!< Array of session listener slots */
	uint16_t listener_count;             /*!< length of the session listener slot array */

	void (*rx_handler)(SBMP_Datagram *dg);  /*!< Datagram receive handler */

	SBMP_FrmInst frm;                /*!< Framing layer internal state */

	// Handshake
	SBMP_HandshakeStatus hsk_status;  /*!< Handshake progress */
	uint16_t hsk_session;            /*!< Session number of the handshake request message */
	uint16_t peer_buffer_size;       /*!< Peer's buffer size (obtained during handshake) */
	SBMP_CksumType peer_pref_cksum;  /*!< Peer's preferred checksum type */

	// Our info for the peer
	uint16_t buffer_size;            /*!< Our buffer size */
	SBMP_CksumType pref_cksum;       /*!< Our preferred checksum */

	SBMP_Datagram static_dg;         /*!< Static datagram, used when DG is pased to a callback.
										  This way the datagram remains valid until next Frm Rx,
										  not only until the callback ends. Disabling the EP in the Rx
										  callback lets you preserve the Dg for a longer period -
										  i.e. put it in a global var, where a loop retrieves it. */
};



/**
 * @brief Initialize the endpoint.
 *
 * @note about the Rx handler:
 * The datagram is valid until a new payload byte is received by the Frm.
 * Disable the endpoint (-> thus also Frm) in the callback if you need
 * to keep the Dg longer. Then re-enable it after the Dg is processed.
 *
 * @param ep          : Endpoint struct pointer, or NULL to allocate one.
 * @param buffer      : Rx buffer. NULL to allocate one.
 * @param buffer_size : Rx buffer length
 * @param dg_rx_handler : Datagram received handler.
 * @param tx_func     : Function to send a byte to USART
 * @return the endpoint struct pointer (allocated if ep was NULL)
 */
SBMP_Endpoint *sbmp_ep_init(SBMP_Endpoint *ep,
							uint8_t *buffer,
							uint16_t buffer_size,
							void (*dg_rx_handler)(SBMP_Datagram *dg),
							void (*tx_func)(uint8_t byte));

/**
 * @brief Configure session listener slots
 * @param ep             : Endpoint pointer
 * @param listener_slots : session listener slots (for multi-message sessions), NULL to malloc.
 * @param slot_count     : number of slots in the array (or to malloc)
 * @return success
 */
bool sbmp_ep_init_listeners(SBMP_Endpoint *ep, SBMP_SessionListenerSlot *listener_slots, uint16_t slot_count);

/**
 * @brief Reset an endpoint and it's Framing Layer
 *
 * This discards all state information.
 *
 * @param ep : Endpoint
 */
void sbmp_ep_reset(SBMP_Endpoint *ep);

/** Set session number (good to randomize before starting a handshake) */
void sbmp_ep_seed_session(SBMP_Endpoint *ep, uint16_t sesn);

/**
 * Set the origin bit.
 * This can be used instead of handshake if origins are pre-assigned.
 */
void sbmp_ep_set_origin(SBMP_Endpoint *endp, bool bit);

/** Set the preferred checksum for this peer. */
void sbmp_ep_set_preferred_cksum(SBMP_Endpoint *endp, SBMP_CksumType cksum_type);

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
bool sbmp_ep_start_message(SBMP_Endpoint *ep, SBMP_DgType type, uint16_t length, uint16_t *sesn_ptr);

/** Send one byte in the current message */
static inline
bool sbmp_ep_send_u8(SBMP_Endpoint *ep, uint8_t byte)
{
	return sbmp_frm_send_byte(&ep->frm, byte);
}

/** Send one 16-bit word in the current message */
bool sbmp_ep_send_u16(SBMP_Endpoint *ep, uint16_t word);

/** Send one 32-bit word in the current message */
bool sbmp_ep_send_u32(SBMP_Endpoint *ep, uint32_t word);


/** Send one byte in the current message */
static inline
bool sbmp_ep_send_i8(SBMP_Endpoint *ep, int8_t byte)
{
	return sbmp_ep_send_u8(ep, ((union pp8){.i8 = byte}).u8);
}

/** send char (just alias) */
static inline
bool sbmp_ep_send_char(SBMP_Endpoint *ep, int8_t byte)
{
	return sbmp_ep_send_i8(ep, byte);
}

/** Send one 16-bit word in the current message */
static inline
bool sbmp_ep_send_i16(SBMP_Endpoint *ep, int16_t word)
{
	return sbmp_ep_send_u16(ep, ((union pp16){.i16 = word}).u16);
}

/** Send one 32-bit word in the current message */
static inline
bool sbmp_ep_send_i32(SBMP_Endpoint *ep, int32_t word)
{
	return sbmp_ep_send_u32(ep, ((union pp32){.i32 = word}).u32);
}

/** Send one float word in the current message */
static inline
bool sbmp_ep_send_float(SBMP_Endpoint *ep, float word)
{
	return sbmp_ep_send_u32(ep, ((union pp32){.f32 = word}).u32);
}

/**
 * @brief Send a data buffer (or a part) in the current message
 *
 * @param ep     : Endpoint struct
 * @param buffer : buffer of bytes to send
 * @param length : buffer length (byte count)
 * @param sent_bytes_ptr : Var to store NR of sent bytes. NULL = don't store.
 * @return actual sent length (until payload is full)
 */
bool sbmp_ep_send_buffer(SBMP_Endpoint *ep, const uint8_t *buffer, uint16_t length, uint16_t *sent_bytes_ptr);

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
 * @brief Claim a session number (+ increment the counter)
 *
 * This can be used if a new session number is needed before starting the session.
 *
 * @param ep : Endpoint struct ptr
 * @return number
 */
uint16_t sbmp_ep_new_session(SBMP_Endpoint *ep);

/**
 * @brief Start a handshake (origin bit arbitration)
 * @param ep : Endpoint struct
 */
bool sbmp_ep_start_handshake(SBMP_Endpoint *ep);

/** Abort current handshake & discard hsk session */
void sbmp_ep_abort_handshake(SBMP_Endpoint *ep);

/** Get current handshake state */
SBMP_HandshakeStatus sbmp_ep_handshake_status(SBMP_Endpoint *ep);

/**
 * @brief Receive a byte from USART (is passed to the framing layer)
 * @param ep   : Endpoint struct
 * @param byte : Byte received from USART
 * @return true if byte was consumed
 */
static inline
SBMP_RxStatus sbmp_ep_receive(SBMP_Endpoint *ep, uint8_t byte)
{
	return sbmp_frm_receive(&ep->frm, byte);
}

/** Enable or disable RX in the FrmInst backing this Endpoint */
static inline
void sbmp_ep_enable_rx(SBMP_Endpoint *ep, bool enable_rx)
{
	sbmp_frm_enable_rx(&ep->frm, enable_rx);
}

/** Enable or disable TX in the FrmInst backing this Endpoint */
static inline
void sbmp_ep_enable_tx(SBMP_Endpoint *ep, bool enable_tx)
{
	sbmp_frm_enable_tx(&ep->frm, enable_tx);
}

/** Enable or disable Rx & TX in the FrmInst backing this Endpoint */
static inline
void sbmp_ep_enable(SBMP_Endpoint *ep, bool enable)
{
	sbmp_frm_enable(&ep->frm, enable);
}

/**
 * @brief Add a session listener
 *
 * The listener will be used for all incoming messages with the given session nr.
 *
 * To unsubscribe, simply remove the listener (possible from within the callback)
 *
 * @param ep       : the endpoint instance
 * @param session  : session number
 * @param callback : datagram handler function
 * @param obj      : Opaque pointer to a data object for the listener.
 *                   Can be used to tell the listener what kind of data is being received,
 *                   if one listener is re-used for multiple transfers.
 * @return success (false if no free slot was found)
 */
bool sbmp_ep_add_listener(SBMP_Endpoint *ep, uint16_t session, SBMP_SessionListener callback, void *obj);

/**
 * Remove a session listener.
 * The caller is expected to free the
 */
void sbmp_ep_remove_listener(SBMP_Endpoint *ep, uint16_t session);

/**
 * Free the slot->obj that is used inside the listener callback.
 * This is useful if the listener is removed outside the listener callback.
 */
void sbmp_ep_free_listener_obj(SBMP_Endpoint *ep, uint16_t session);


#endif /* SBMP_SESSION_H */

#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>


/** Checksum types */
typedef enum {
	SBMP_CKSUM_NONE = 0,   /*!< No checksum */
	SBMP_CKSUM_CRC32 = 32, /*!< ISO CRC-32 */
} SBMP_ChecksumType;

/**
 * SBMP message object. Allocated for each incoming and outgoing message.
 *
 * Processing code is responsible for freeing the object, including the buffer.
 */
typedef struct {
	uint8_t *_backing_buffer; /*!< Backing malloc'd buffer. Must be freed when freeing the datagram! */
	uint8_t *payload;        /*!< Datagram payload */
	uint8_t  type;   /*!< Datagram type ID */
	size_t length; /*!< Datagram length (bytes) */
	uint16_t session;  /*!< Datagram session number */
} SBMP_Datagram;

/** Internal SBMP state */
typedef struct SBMP_State_struct SBMP_State;

/**
 * @brief Allocate & initialize the SBMP internal state struct
 *
 * @param frame_handler : frame rx callback.
 * @param buffer_size : size of the payload buffer
 * @return pointer to the allocated struct
 */
SBMP_State *sbmp_init(
	void (*rx_handler)(uint8_t *payload, size_t length),
	void (*tx_func)(uint8_t byte),
	size_t buffer_size
);

/**
 * @brief De-init the SBMP state structure & free all allocated memory
 *        (including the buffer).
 * @param state : the state struct
 */
void sbmp_destroy(SBMP_State *state);

/**
 * @brief Handle an incoming byte
 *
 * @param state  : SBMP state struct
 * @param rxbyte : byte received
 */
void sbmp_receive(SBMP_State *state, uint8_t rxbyte);

/**
 * @brief Start a frame transmission
 *
 * @param state : SBMP state struct
 * @param cksum_type : checksum to use (0, 32)
 * @param length : payload length
 * @return true if frame was started.
 */
bool sbmp_transmit_start(SBMP_State *state, SBMP_ChecksumType cksum_type, size_t length);

/**
 * @brief Send one byte in the open frame.
 *
 * If this is the last payload byte, the checksum (if requested)
 * is sent, and the transmitter enters IDLE mode.
 *
 * @param state : SBMP state struct
 * @param byte : byte to send
 * @return true on success (value did fit in a frame)
 */
bool sbmp_transmit_byte(SBMP_State *state, uint8_t byte);

/**
 * @brief Send a data buffer (or a part).
 *
 * If the payload is completed, checksum will be added and
 * the transmitter enters IDLE mode.
 *
 * @param state : SBMP state struct
 * @param buffer : buffer of bytes to send
 * @param length : buffer length (byte count)
 * @return actual sent length (until payload is full)
 */
size_t sbmp_transmit_buffer(SBMP_State *state, const uint8_t *buffer, size_t length);

/**
 * @brief Convert a received buffer payload to a datagram.
 *
 * If the payload is < 3 bytes long, datagram can't be createdn and NULL
 * is returned instead. The caller should then free the payload buffer.
 *
 * @attention
 * The datagram has to be destroyed using sbmp_destroy_datagram() when
 * no longer needed, to free the used memory. This also frees the payload buffer.
 *
 * @param payload : frame payload to parse
 * @param length  : payload length
 * @return allocated datagram backed by the frame payload buffer.
 */
SBMP_Datagram *sbmp_parse_datagram(uint8_t *rx_payload, size_t length);

/**
 * @brief Free all memory used by a datagram and the backing payload buffer.
 *
 * @param dg : datagram
 */
void sbmp_destroy_datagram(SBMP_Datagram *dg);

/** Start a datagram transmission */
bool sbmp_datagram_start(SBMP_State *state, SBMP_ChecksumType cksum_type, uint16_t session, uint8_t type, size_t length);

/** Send a byte in the datagram */
bool sbmp_datagram_add_byte(SBMP_State *state, uint8_t byte);

/** Send a buffer in the datagram */
size_t sbmp_datagram_add_buffer(SBMP_State *state, const uint8_t *payload, size_t length);

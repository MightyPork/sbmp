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
	uint8_t *datagram;        /*!< Datagram payload */
	uint8_t  datagram_type;   /*!< Datagram type ID */
	size_t datagram_length; /*!< Datagram length (bytes) */
	uint16_t session_number;  /*!< Datagram session number */
} SBMP_Datagram;

/** Internal SBMP state */
typedef struct SBMP_State_struct SBMP_State;

/**
 * @brief Handle an incoming byte
 * @param state  : SBMP receiver state struct
 * @param rxbyte : byte received
 */
void sbmp_receive(SBMP_State *state, uint8_t rxbyte);

/**
 * @brief Allocate & initialize the SBMP internal state struct
 * @param frame_handler : frame rx callback.
 * @param buffer_size : size of the payload buffer
 * @return pointer to the allocated struct
 */
SBMP_State *sbmp_init(
	void (*frame_handler)(uint8_t *payload, size_t length),
	size_t buffer_size
);

/**
 * @brief De-init the SBMP state structure & free all allocated memory
 *        (including the buffer).
 * @param state : the state struct
 */
void sbmp_destroy(SBMP_State *state);

/**
 * @brief Convert a buffer payload to a datagram.
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
SBMP_Datagram *sbmp_parse_datagram(uint8_t *payload, size_t length);

/**
 * @brief Free all memory used by a datagram and the backing payload buffer.
 *
 * @param dg : datagram
 */
void sbmp_destroy_datagram(SBMP_Datagram *dg);

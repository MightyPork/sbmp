#ifndef SBMP_DATAGRAM_H
#define SBMP_DATAGRAM_H

/**
 * SBMP datagram utilities
 *
 * The transmit functions require the framing layer.
 */

#include <stdint.h>

#include "sbmp_common.h"
#include "sbmp_frame.h"

/**
 * SBMP datagram object.
 */
typedef struct {
	const uint8_t *payload;        /*!< Datagram payload */
	uint8_t  type;   /*!< Datagram type ID */
	uint16_t length; /*!< Datagram length (bytes) */
	uint16_t session;  /*!< Datagram session number */
} SBMP_Datagram;


/**
 * @brief Convert a received buffer payload to a datagram.
 *
 * If the payload is < 3 bytes long, datagram can't be createdn and NULL
 * is returned instead. The caller should then free the payload buffer.
 *
 * @param dg            : datagram variable to populate
 * @param frame_payload : frame payload to parse
 * @param length        : frame payload length
 * @return allocated datagram backed by the frame payload buffer.
 */
bool sbmp_parse_datagram(SBMP_Datagram *dg, const uint8_t *frame_payload, uint16_t length);

/**
 * @brief Start a datagram (and the frame)
 *
 * @param state      : SBMP state struct
 * @param cksum_type : Checksum type to use for the frame; 0 - none, 32 - CRC32
 * @param session    : session number
 * @param type       : Datagram type ID
 * @param length     : Datagram payload length (bytes)
 * @return success
 */
bool sbmp_start_datagram(SBMP_State *state, SBMP_ChecksumType cksum_type, uint16_t session, uint8_t type, uint16_t length);

/**
 * @brief Send a complete prepared datagram, also starts the frame.
 *
 * @param state      : SBMP state struct
 * @param cksum_type : Checksum type to use for the frame; 0 - none, 32 - CRC32
 * @param dg         : Datagram struct containing DG settings and the payload.
 * @return success
 */
bool sbmp_send_datagram(SBMP_State *state, SBMP_ChecksumType cksum_type, SBMP_Datagram *dg);


#endif /* SBMP_DATAGRAM_H */

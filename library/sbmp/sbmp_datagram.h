#ifndef SBMP_DATAGRAM_H
#define SBMP_DATAGRAM_H

/**
 * SBMP datagram layer
 *
 * This layer is coupled to the framing layer, and
 * builds a datagram abstraction on top of it.
 *
 * This layer sits right under the session layer,
 * and is not very useful on it's own.
 */

#include <stdint.h>

#include "sbmp.h"

typedef uint8_t SBMP_DgType;
#define SBMP_DG_HSK_START    0x00
#define SBMP_DG_HSK_ACCEPT   0x01
#define SBMP_DG_HSK_CONFLICT 0x02

/**
 * SBMP datagram object.
 */
typedef struct {
	const uint8_t *payload; /*!< Datagram payload */
	SBMP_DgType type;       /*!< Datagram type ID */
	uint16_t length;        /*!< Datagram length (bytes) */
	uint16_t session;       /*!< Datagram session number */
} SBMP_Datagram;


/**
 * @brief Convert a received buffer payload to a datagram.
 *
 * If the payload is < 3 bytes long, datagram can't be createdn and NULL
 * is returned instead. The caller should then free the payload buffer.
 *
 * @param dg            : datagram variable to populate, or NULL to allocate
 * @param frame_payload : frame payload to parse
 * @param length        : frame payload length
 * @return datagram (allocated if dg was NULL), or NULL if parsing failed.
 */
SBMP_Datagram *sbmp_parse_datagram(SBMP_Datagram *dg_or_null, const uint8_t *frame_payload, uint16_t length);

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
bool sbmp_start_datagram(SBMP_FrmState *state, SBMP_ChecksumType cksum_type, uint16_t session, SBMP_DgType type, uint16_t length);

/**
 * @brief Send a complete prepared datagram, also starts the frame.
 *
 * @param state      : SBMP state struct
 * @param cksum_type : Checksum type to use for the frame; 0 - none, 32 - CRC32
 * @param dg         : Datagram struct containing DG settings and the payload.
 * @return success
 */
bool sbmp_send_datagram(SBMP_FrmState *state, SBMP_ChecksumType cksum_type, SBMP_Datagram *dg);


#endif /* SBMP_DATAGRAM_H */

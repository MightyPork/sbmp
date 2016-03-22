#ifndef SBMP_BULK_H
#define SBMP_BULK_H

/**
 * Utilities for the chunked bulk data transfer.
 * Builds on top of the session layer.
 */

#include "sbmp_config.h"
#include "sbmp_datagram.h"
#include "sbmp_session.h"

/**
 * @brief Offer a bulk data transfer.
 * @param ep
 * @param bulk_length : total bulk data length
 * @param xtra        : extra data in the datagram
 * @param xtra_len    : extra data length
 * @param sesn        : session number to use
 * @return send success
 */
bool sbmp_bulk_offer(SBMP_Endpoint *ep, uint32_t bulk_length, const uint8_t *xtra, uint16_t xtra_len, uint16_t sesn);

/**
 * @brief Request a chunk of the bulk data.
 * @param ep
 * @param offset     : offset of the chunk
 * @param chunk_size : length of the chunk in bytes
 * @param sesn       : session nr to use
 * @return send success
 */
bool sbmp_bulk_request(SBMP_Endpoint *ep, uint32_t offset, uint16_t chunk_size, uint16_t sesn);

/**
 * @brief Send a chunk of data as requested.
 *
 * NOTE: If needed, you can also simply start the response and send the data byte-by-byte
 * or in 32-bit chunks or whatever suits your needs best.
 *
 * There is no need to close the datagram, it's closed automatically with the last byte.
 *
 * @param ep
 * @param chunk      : buffer containing a chunk of the data
 * @param chunk_size : length of the chunk in bytes
 * @param sesn       : session nr to use
 * @return send success
 */
bool sbmp_bulk_send_data(SBMP_Endpoint *ep, const uint8_t *chunk, uint16_t chunk_len, uint16_t sesn);

/**
 * @brief Abort the bulk transfer
 *
 * @param ep
 * @param sesn : session nr to use
 * @return send success
 */
bool sbmp_bulk_abort(SBMP_Endpoint *ep, uint16_t sesn);


#endif // SBMP_BULK_H

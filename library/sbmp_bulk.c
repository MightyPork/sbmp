#include <stdlib.h>
#include <stdint.h>

#include "sbmp_config.h"
#include "sbmp_logging.h"
#include "sbmp_datagram.h"
#include "sbmp_session.h"
#include "sbmp_bulk.h"


/** Offer a bulk data transfer. */
bool sbmp_bulk_offer(SBMP_Endpoint *ep, uint32_t bulk_length, const uint8_t *user_data, uint16_t user_data_len, uint16_t sesn)
{
	return sbmp_ep_start_response(ep, DG_BULK_OFFER, user_data_len + 4, sesn) // using response because it allows to use a custom sesn
		   && sbmp_ep_send_u32(ep, bulk_length)
		   && sbmp_ep_send_buffer(ep, user_data, user_data_len, NULL);
}

/** Request a chunk of the bulk data. */
bool sbmp_bulk_request(SBMP_Endpoint *ep, uint32_t offset, uint16_t chunk_size, uint16_t sesn)
{
	return sbmp_ep_start_response(ep, DG_BULK_OFFER, 4 + 2, sesn)
		   && sbmp_ep_send_u32(ep, offset)
		   && sbmp_ep_send_u16(ep, chunk_size);
}

/** Send a chunk of data as requested. */
bool sbmp_bulk_send_data(SBMP_Endpoint *ep, const uint8_t *chunk, uint16_t chunk_len, uint16_t sesn)
{
	return sbmp_ep_start_response(ep, DG_BULK_DATA, chunk_len, sesn)
		   && sbmp_ep_send_buffer(ep, chunk, chunk_len, NULL);
}


/** Abort the bulk transfer. */
bool sbmp_bulk_abort(SBMP_Endpoint *ep, uint16_t sesn)
{
	// 0-byte response is just the header.
	return sbmp_ep_start_response(ep, DG_BULK_ABORT, 0, sesn);
}


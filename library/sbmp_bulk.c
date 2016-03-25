#include <stdlib.h>
#include <stdint.h>

#include "sbmp_config.h"
#include "sbmp_datagram.h"
#include "sbmp_session.h"
#include "sbmp_bulk.h"


/** Offer a bulk data transfer. */
bool sbmp_bulk_offer(SBMP_Endpoint *ep, uint32_t bulk_length, const uint8_t *xtra_data, uint16_t xtra_data_len, uint16_t sesn)
{
	bool suc = sbmp_ep_start_response(ep, DG_BULK_OFFER, xtra_data_len + sizeof(uint32_t), sesn) // using response because it allows to use a custom sesn
			   && sbmp_ep_send_u32(ep, bulk_length)
			   && sbmp_ep_send_buffer(ep, xtra_data, xtra_data_len, NULL);

	if (suc)sbmp_dbg("Bulk OFFER sent, len %d; sesn %d", bulk_length, sesn);
	return suc;
}

/** Request a chunk of the bulk data. */
bool sbmp_bulk_request(SBMP_Endpoint *ep, uint32_t offset, uint16_t chunk_size, uint16_t sesn)
{
	bool suc = sbmp_ep_start_response(ep, DG_BULK_REQUEST, sizeof(uint32_t) + sizeof(uint16_t), sesn)
			   && sbmp_ep_send_u32(ep, offset)
			   && sbmp_ep_send_u16(ep, chunk_size);

	if (suc)sbmp_dbg("Bulk REQUEST sent, offs %d, chunk %d; sesn %d", offset, chunk_size, sesn);
	return suc;
}

/** Send a chunk of data as requested. */
bool sbmp_bulk_send_data(SBMP_Endpoint *ep, const uint8_t *chunk, uint16_t chunk_len, uint16_t sesn)
{
	bool suc = sbmp_ep_start_response(ep, DG_BULK_DATA, chunk_len, sesn)
			   && sbmp_ep_send_buffer(ep, chunk, chunk_len, NULL);

	if (suc)sbmp_dbg("Bulk DATA sent, len %d; sesn %d", chunk_len, sesn);
	return suc;
}


/** Abort the bulk transfer. */
bool sbmp_bulk_abort(SBMP_Endpoint *ep, uint16_t sesn)
{
	// 0-byte response is just the header.
	bool suc = sbmp_ep_start_response(ep, DG_BULK_ABORT, 0, sesn);

	if (suc) sbmp_dbg("Bulk transfer aborted; sesn %d", sesn);
	return suc;
}


#include <stdlib.h>
#include <stdint.h>

#include "sbmp_config.h"
#include "sbmp_logging.h"
#include "sbmp_datagram.h"

SBMP_Datagram *sbmp_parse_datagram(SBMP_Datagram *dg, const uint8_t *payload, uint16_t length)
{
	if (length < 3) {
		sbmp_error("Can't parse datagram, payload too short.");
		return NULL; // shorter than the minimal no-payload datagram
	}

	// S.N. (2 B) | Dg Type (1 B) | Payload

	if (dg == NULL) {
		// request to allocate
		#if SBMP_MALLOC
			dg = malloc(sizeof(SBMP_Datagram));
		#else
			return NULL; // fail
		#endif
	}

	dg->session = (uint16_t)((payload[0]) | (payload[1] << 8));
	dg->type = payload[2];
	dg->length = length - 3;
	dg->payload = payload + 3; // pointer arith

	return dg;
}


/** Start a datagram transmission */
bool sbmp_start_datagram(SBMP_FrmInst *state, SBMP_ChecksumType cksum_type, uint16_t session, SBMP_DgType type, uint16_t length)
{
	if (length > (0xFFFF - 3)) {
		sbmp_error("Can't send a datagram, payload too long.");
		return false;
	}

	if (state->tx_state != FRM_STATE_IDLE) {
		sbmp_error("Can't state datagram, SBMP tx not IDLE.");
		return false;
	}

	if (! sbmp_frm_start(state, cksum_type, length + 3)) return false;

	sbmp_frm_send_byte(state, session & 0xFF);
	sbmp_frm_send_byte(state, (session >> 8) & 0xFF);
	sbmp_frm_send_byte(state, type);

	return true;
}


/** Send a whole datagram in one go */
bool sbmp_send_datagram(SBMP_FrmInst *state, SBMP_ChecksumType cksum_type, SBMP_Datagram *dg)
{
	if (! sbmp_start_datagram(state, cksum_type, dg->session, dg->type, dg->length)) {
		sbmp_error("Failed to start datagram.");
		return false;
	}

	size_t n = sbmp_frm_send_buffer(state, dg->payload, dg->length);
	return (n == dg->length);
}

#include <stdlib.h>
#include <stdint.h>

#include "sbmp_config.h"
#include "sbmp_logging.h"
#include "sbmp_datagram.h"

SBMP_Datagram *sbmp_dg_parse(SBMP_Datagram *dg, const uint8_t *payload, uint16_t length)
{
	if (length < 3) {
		sbmp_error("Can't parse datagram, payload too short.");
		return NULL; // shorter than the minimal no-payload datagram
	}

	// S.N. (2 B) | Dg Type (1 B) | Payload

#if SBMP_MALLOC
	if (dg == NULL) {
	// request to allocate
		dg = malloc(sizeof(SBMP_Datagram));
	}
#else
	if (dg == NULL) {
		return NULL; // fail
	}
#endif

	dg->session = (uint16_t)((payload[0]) | (payload[1] << 8));
	dg->type = payload[2];
	dg->length = length - 3;
	dg->payload = payload + 3; // pointer arith

	return dg;
}


/** Start a datagram transmission */
bool sbmp_dg_start(SBMP_FrmInst *frm, SBMP_CksumType cksum_type, uint16_t session, SBMP_DgType type, uint16_t length)
{
	if (length > (0xFFFF - 3)) {
		sbmp_error("Can't send a datagram, payload too long.");
		return false;
	}

	if (frm->tx_status != FRM_STATE_IDLE) {
		sbmp_error("Can't state datagram, SBMP tx not IDLE.");
		return false;
	}

	if (! sbmp_frm_start(frm, cksum_type, length + 3)) return false;

	sbmp_frm_send_byte(frm, session & 0xFF);
	sbmp_frm_send_byte(frm, (session >> 8) & 0xFF);
	sbmp_frm_send_byte(frm, type);

	return true;
}


/** Send a whole datagram in one go */
bool sbmp_dg_send(SBMP_FrmInst *frm, SBMP_CksumType cksum_type, SBMP_Datagram *dg)
{
	if (! sbmp_dg_start(frm, cksum_type, dg->session, dg->type, dg->length)) {
		sbmp_error("Failed to start datagram.");
		return false;
	}

	size_t n = sbmp_frm_send_buffer(frm, dg->payload, dg->length);
	return (n == dg->length);
}

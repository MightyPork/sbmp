#include "payload_parser.h"

uint8_t pp_u8(PayloadParser *pp)
{
	return pp->buf[pp->ptr++];
}

uint16_t pp_u16(PayloadParser *pp)
{
	uint16_t x = pp->buf[pp->ptr++];
	x |= (uint16_t)(pp->buf[pp->ptr++] << 8);
	return x;
}

uint32_t pp_u32(PayloadParser *pp)
{
	uint32_t x = pp->buf[pp->ptr++];
	x |= (uint32_t)(pp->buf[pp->ptr++] << 8);
	x |= (uint32_t)(pp->buf[pp->ptr++] << 16);
	x |= (uint32_t)(pp->buf[pp->ptr++] << 24);
	return x;
}

uint8_t *pp_rest(PayloadParser *pp, size_t *length)
{
	if (length != NULL)	*length = pp->len - pp->ptr;
	return pp->buf + pp->ptr; // pointer arith
}

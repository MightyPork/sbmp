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

int8_t pp_i8(PayloadParser *pp)
{
	uint8_t x = pp_u8(pp);
	return *(int8_t *) &x;
}

char pp_char(PayloadParser *pp)
{
	return pp_i8(pp);
}

int16_t pp_i16(PayloadParser *pp)
{
	uint16_t x = pp_u16(pp);
	return *(int16_t *) &x;
}

int32_t pp_i32(PayloadParser *pp)
{
	uint32_t x = pp_u32(pp);
	return *(int32_t *) &x;
}

float pp_float(PayloadParser *pp)
{
	uint32_t x = pp_u32(pp);
	return *(float *) &x;
}

#include "sbmp_config.h"
#include "payload_parser.h"

#define check_capacity(pp_p, needed) \
	if ((pp_p)->ptr + (needed) > (pp_p)->len) { \
		pp_overflow(); \
		return 0; \
	}

static void FLASH_FN pp_overflow(void)
{
	sbmp_error("PayloadParser out of range.");
}


uint8_t FLASH_FN pp_u8(PayloadParser *pp)
{
	check_capacity(pp, sizeof(uint8_t));

	return pp->buf[pp->ptr++];
}

uint16_t FLASH_FN pp_u16(PayloadParser *pp)
{
	check_capacity(pp, sizeof(uint16_t));

	uint16_t x = pp->buf[pp->ptr++];
	x |= (uint16_t)(pp->buf[pp->ptr++] << 8);
	return x;
}

uint32_t FLASH_FN pp_u32(PayloadParser *pp)
{
	check_capacity(pp, sizeof(uint32_t));

	uint32_t x = pp->buf[pp->ptr++];
	x |= (uint32_t)(pp->buf[pp->ptr++] << 8);
	x |= (uint32_t)(pp->buf[pp->ptr++] << 16);
	x |= (uint32_t)(pp->buf[pp->ptr++] << 24);
	return x;
}

const FLASH_FN uint8_t *pp_rest(PayloadParser *pp, size_t *length)
{
	int len = (int)(pp->len - pp->ptr);
	if (len <= 0) {
		if (length != NULL) *length = 0;
		return NULL;
	}

	if (length != NULL)	{
		*length = (size_t)len;
	}

	return pp->buf + pp->ptr; // pointer arith
}

/** Read int8_t from the payload. */
int8_t FLASH_FN pp_i8(PayloadParser *pp)
{
	return ((union conv8){.u8 = pp_u8(pp)}).i8;
}

/** Read int16_t from the payload. */
int16_t FLASH_FN pp_i16(PayloadParser *pp)
{
	return ((union conv16){.u16 = pp_u16(pp)}).i16;
}

/** Read int32_t from the payload. */
int32_t FLASH_FN pp_i32(PayloadParser *pp)
{
	return ((union conv32){.u32 = pp_u32(pp)}).i32;
}

/** Read 4-byte float from the payload. */
float FLASH_FN pp_float(PayloadParser *pp)
{
	return ((union conv32){.u32 = pp_u32(pp)}).f32;
}

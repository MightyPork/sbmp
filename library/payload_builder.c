#include "sbmp_config.h"
#include "payload_builder.h"
#include "type_coerce.h"

#define check_capacity(pb_p, needed) \
	if ((pb_p)->ptr + (needed) > (pb_p)->capacity) { \
		pb_overflow(); \
		return false; \
	}


static FLASH_FN void pb_overflow(void)
{
	sbmp_error("PayloadBuilder overflow.");
}

/** Write uint8_t to the buffer */
bool FLASH_FN pb_u8(PayloadBuilder *pb, uint8_t byte)
{
	check_capacity(pb, sizeof(uint8_t));
	pb->buf[pb->ptr++] = byte;
	return true;
}

/** Write uint16_t to the buffer. */
bool FLASH_FN pb_u16(PayloadBuilder *pb, uint16_t word)
{
	check_capacity(pb, sizeof(uint16_t));
	pb->buf[pb->ptr++] = word & 0xFF;
	pb->buf[pb->ptr++] = (word >> 8) & 0xFF;
	return true;
}

/** Write uint32_t to the buffer. */
bool FLASH_FN pb_u32(PayloadBuilder *pb, uint32_t word)
{
	check_capacity(pb, sizeof(uint32_t));
	pb->buf[pb->ptr++] = word & 0xFF;
	pb->buf[pb->ptr++] = (word >> 8) & 0xFF;
	pb->buf[pb->ptr++] = (word >> 16) & 0xFF;
	pb->buf[pb->ptr++] = (word >> 24) & 0xFF;
	return true;
}

/** Write int8_t to the buffer. */
bool FLASH_FN pb_i8(PayloadBuilder *pb, int8_t byte)
{
	return pb_u8(pb, ((union conv8){.i8 = byte}).u8);
}

/** Write int16_t to the buffer. */
bool FLASH_FN pb_i16(PayloadBuilder *pb, int16_t word)
{
	return pb_u16(pb, ((union conv16){.i16 = word}).u16);
}

/** Write int32_t to the buffer. */
bool FLASH_FN pb_i32(PayloadBuilder *pb, int32_t word)
{
	return pb_u32(pb, ((union conv32){.i32 = word}).u32);
}

/** Write 4-byte float to the buffer. */
bool FLASH_FN pb_float(PayloadBuilder *pb, float f)
{
	return pb_u32(pb, ((union conv32){.f32 = f}).u32);
}

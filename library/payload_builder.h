#ifndef PAYLOAD_BUILDER_H
#define PAYLOAD_BUILDER_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/**
 * This module can help you build payload arrays
 *
 * The builder performs bounds checking and prints error
 * message on overflow for easy debugging.
 */

typedef struct {
	uint8_t *buf;
	size_t ptr;
	size_t capacity;
} PayloadBuilder;

#define pb_start(buf, capacity) ((PayloadBuilder){buf, 0, capacity})
#define pb_length(pb_p) ((pb_p)->ptr)

/** Write uint8_t to the buffer */
bool pb_u8(PayloadBuilder *pb, uint8_t byte);

/** Write uint16_t to the buffer. */
bool pb_u16(PayloadBuilder *pb, uint16_t word);

/** Write uint32_t to the buffer. */
bool pb_u32(PayloadBuilder *pb, uint32_t word);

/** Write char (int8_t) to the buffer. */
#define pb_char(pb, c) pb_i8(pb, c)

/** Write int8_t to the buffer. */
bool pb_i8(PayloadBuilder *pb, int8_t byte);

/** Write int16_t to the buffer. */
bool pb_i16(PayloadBuilder *pb, int16_t word);

/** Write int32_t to the buffer. */
bool pb_i32(PayloadBuilder *pb, int32_t word);

/** Write 4-byte float to the buffer. */
bool pb_float(PayloadBuilder *pb, float f);

#endif // PAYLOAD_BUILDER_H

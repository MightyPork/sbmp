#ifndef PAYLOAD_PARSER_H
#define PAYLOAD_PARSER_H

/**
 * Paylod parser is used to extract values fron a byte buffer.
 *
 * It's a set of simple routines that help you extract data from
 * a SBMP datagram payload.
 *
 * The functions don't do any bounds checking, so be careful.
 */

#include <stdint.h>
#include <stddef.h>

typedef struct {
	uint8_t *buf;
	size_t ptr;
	size_t len;
} PayloadParser;


/** Start the parser. This is assigned to a local variable. */
#define pp_start(buf, length) {.buf = (buf), .ptr = 0, .len = (length)}

/**
 * @brief Get the remainder of the buffer.
 *
 * @param pp
 * @param length : here the buffer length will be stored. NULL to do not store.
 * @return the remaining portion of the input buffer
 */
uint8_t *pp_rest(PayloadParser *pp, size_t *length);

/** Read uint8_t from the payload. */
uint8_t pp_u8(PayloadParser *pp);

/** Read uint16_t from the payload. */
uint16_t pp_u16(PayloadParser *pp);

/** Read uint32_t from the payload. */
uint32_t pp_u32(PayloadParser *pp);

/** Read int8_t from the payload. */
static inline
int8_t pp_i8(PayloadParser *pp)
{
	uint8_t x = pp_u8(pp);
	return *(int8_t *) &x;
}

/** Read char (int8_t) from the payload. */
#define pp_char(pp) pp_i8(pp)

/** Read int16_t from the payload. */
static inline
int16_t pp_i16(PayloadParser *pp)
{
	uint16_t x = pp_u16(pp);
	return *(int16_t *) &x;
}

/** Read int32_t from the payload. */
static inline
int32_t pp_i32(PayloadParser *pp)
{
	uint32_t x = pp_u32(pp);
	return *(int32_t *) &x;
}

/** Read 4-byte float from the payload. */
static inline
float pp_float(PayloadParser *pp)
{
	uint32_t x = pp_u32(pp);
	return *(float *) &x;
}

#endif // PAYLOAD_PARSER_H

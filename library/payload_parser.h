#ifndef PAYLOAD_PARSER_H
#define PAYLOAD_PARSER_H

/**
 * Paylod parser is used to extract values fron a byte buffer.
 *
 * It's a set of simple routines that help you extract data from
 * a SBMP datagram payload. The payload parser should take care
 *  of endianness of the SBMP encoding.
 *
 * The functions don't do any bounds checking, so be careful.
 */

#include "esp8266.h"
#include <stddef.h>

typedef struct {
	const uint8_t *buf;
	size_t ptr;
	size_t len;
} PayloadParser;


// Structs for conversion between types

union pp8 {
	uint8_t u8;
	int8_t i8;
};

union pp16 {
	uint16_t u16;
	int16_t i16;
};

union pp32 {
	uint32_t u32;
	int32_t i32;
	float f32;
};


/** Start the parser. This is assigned to a local variable. */
#define pp_start(buffer, length) ((PayloadParser){.buf = (buffer), .ptr = 0, .len = (length)})

/**
 * @brief Get the remainder of the buffer.
 *
 * @param pp
 * @param length : here the buffer length will be stored. NULL to do not store.
 * @return the remaining portion of the input buffer
 */
const uint8_t *pp_rest(PayloadParser *pp, size_t *length);

/** Read uint8_t from the payload. */
uint8_t pp_u8(PayloadParser *pp);

/** Read uint16_t from the payload. */
uint16_t pp_u16(PayloadParser *pp);

/** Read uint32_t from the payload. */
uint32_t pp_u32(PayloadParser *pp);

/** Read char (int8_t) from the payload. */
#define pp_char(pp) pp_i8(pp)

/** Read int8_t from the payload. */
int8_t pp_i8(PayloadParser *pp);

/** Read int16_t from the payload. */
int16_t pp_i16(PayloadParser *pp);

/** Read int32_t from the payload. */
int32_t pp_i32(PayloadParser *pp);

/** Read 4-byte float from the payload. */
float pp_float(PayloadParser *pp);

#endif // PAYLOAD_PARSER_H

#ifndef PAYLOAD_PARSER_H
#define PAYLOAD_PARSER_H

/**
 * Paylod parser is used to extract values fron a byte buffer.
 *
 * It's a set of simple routines that help you extract data from
 * a SBMP datagram payload. The payload parser should take care
 * of endianness of the SBMP encoding.
 *
 * The parser functions will print an error if you reach the end of data,
 * and return zero. This can help you with debugging.
 */

#include "esp8266.h"
#include <stddef.h>
#include "type_coerce.h"

typedef struct {
	const uint8_t *buf;
	size_t ptr;
	size_t len;
} PayloadParser;


/** Start the parser. This is assigned to a local variable. */
#define pp_start(buffer, length) ((PayloadParser){buffer, 0, length})

/**
 * @brief Get the remainder of the buffer.
 *
 * Returns NULL and sets 'length' to 0 if there are no bytes left.
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

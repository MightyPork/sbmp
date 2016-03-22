#ifndef PAYLOAD_PARSER_H
#define PAYLOAD_PARSER_H

#include <stdint.h>
#include <stddef.h>

typedef struct {
	uint8_t *buf;
	size_t ptr;
} PayloadParser;


#define pp_start(buf) {.buf = buf, .ptr = 0}

uint8_t pp_u8(PayloadParser *pp);

uint16_t pp_u16(PayloadParser *pp);

uint32_t pp_u32(PayloadParser *pp);

int8_t pp_i8(PayloadParser *pp);

char pp_char(PayloadParser *pp);

int16_t pp_i16(PayloadParser *pp);

int32_t pp_i32(PayloadParser *pp);

float pp_float(PayloadParser *pp);



#endif // PAYLOAD_PARSER_H

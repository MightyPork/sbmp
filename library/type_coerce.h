#ifndef TYPE_COERCE_H
#define TYPE_COERCE_H

#include <stdint.h>
#include <stddef.h>

// Structs for conversion between types

union conv8 {
	uint8_t u8;
	int8_t i8;
};

union conv16 {
	uint16_t u16;
	int16_t i16;
};

union conv32 {
	uint32_t u32;
	int32_t i32;
	float f32;
};

#endif // TYPE_COERCE_H

#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

uint32_t crc32buf(uint8_t *buf, size_t len);

uint32_t crc32_begin(void);
void crc32_update(uint32_t *old_crc, uint8_t ch);
uint32_t crc32_end(uint32_t old_crc);

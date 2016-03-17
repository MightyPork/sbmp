#ifndef CRC32_H
#define CRC32_H

#include "sbmp_config.h"
#if SBMP_HAS_CRC32

/**
 * This module is used by the framing layer to calculate CRC32.
 *
 * If your hardware provides a hardware checksum calculator,
 * you can modify the .c file to use that instead of
 * doing it in software.
 */

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/**
 * @brief Calculate CRC32 of a buffer.
 * @param buf : buffer to checksum
 * @param len : buffer size
 * @return the CRC32 checksum
 */
uint32_t crc32buf(uint8_t *buf, size_t len);

/**
 * @brief Start calculating a checksum of a block of data.
 * @return the scratch value, for use in the update and end functions.
 */
uint32_t crc32_begin(void);

/**
 * @brief Update the CRC32 scratch value with a byte of the data block.
 * @param crc_scratch : old scratch value pointer.
 * @param b : received byte
 * @return updated scratch
 */
uint32_t crc32_update(uint32_t crc_scratch, uint8_t b);

/**
 * @brief Finish the CRC calculation.
 * @param crc_scratch : your scratch buffer.
 * @return the final CRC32 value.
 */
uint32_t crc32_end(uint32_t crc_scratch);


#endif /* SBMP_HAS_CRC32 */

#endif /* CRC32_H */

#ifndef SBMP_CHECKSUM_H
#define SBMP_CHECKSUM_H

/**
 * Checksum functions for the SBMP framing layer.
 */

#include <stdint.h>
#include <stdbool.h>

#include "sbmp_config.h"


/** Checksum types */
typedef enum {
	SBMP_CKSUM_NONE = 0,   /*!< No checksum */
	SBMP_CKSUM_CRC32 = 32, /*!< ISO CRC-32 */
	SBMP_CKSUM_XOR = 1,    /*!< Simple XOR check, good for small micros (Arduino) */

} SBMP_CksumType;


/** Get nr of bytes in a checksum */
uint8_t checksum_length(SBMP_CksumType cksum_type);

/** Start calculating a checksum. Updates scratch. */
void cksum_begin(SBMP_CksumType type, uint32_t *scratch);

/** Update the checksum calculation with an incoming byte. Updates scratch. */
void cksum_update(SBMP_CksumType type, uint32_t *scratch, uint8_t byte);

/** Stop the checksum calculation, get the result */
void cksum_end(SBMP_CksumType type, uint32_t *scratch);

/** Check if the calculated checksum matches the received one */
bool cksum_verify(SBMP_CksumType type, uint32_t *scratch, uint32_t received_cksum);


#endif /* SBMP_CHECKSUM_H */

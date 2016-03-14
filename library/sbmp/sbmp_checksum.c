#include <stdint.h>
#include <stdbool.h>

#include "sbmp_config.h"
#include "sbmp_checksum.h"

#if SBMP_HAS_CRC32
#include "crc32.h"
#endif


/** Get nr of bytes in a checksum */
uint8_t checksum_length(SBMP_ChecksumType cksum_type)
{
	switch (cksum_type) {
		case SBMP_CKSUM_CRC32: return 4;
		case SBMP_CKSUM_XOR:   return 1;
		case SBMP_CKSUM_NONE:  return 0;
		default:
			return 4; // assume all unknown checksums are 4 bytes long
	}
}

/** Start calculating a checksum */
void cksum_begin(SBMP_ChecksumType type, uint32_t *scratch)
{
	switch (type) {

#if SBMP_HAS_CRC32
		case SBMP_CKSUM_CRC32:
			*scratch = crc32_begin();
			break;
#endif
		case SBMP_CKSUM_XOR:
			*scratch = 0;
			break;

		case SBMP_CKSUM_NONE: // fall-through
		default:
			;
	}
}

/** Update the checksum calculation with an incoming byte */
void cksum_update(SBMP_ChecksumType type, uint32_t *scratch, uint8_t byte)
{
	switch (type) {

#if SBMP_HAS_CRC32
		case SBMP_CKSUM_CRC32:
			*scratch = crc32_update(*scratch, byte);
			break;
#endif
		case SBMP_CKSUM_XOR:
			*scratch ^= byte;
			break;

		case SBMP_CKSUM_NONE: // fall-through
		default:
			;
	}
}

/** Stop the checksum calculation, get the result */
void cksum_end(SBMP_ChecksumType type, uint32_t *scratch)
{
	switch (type) {

#if SBMP_HAS_CRC32
		case SBMP_CKSUM_CRC32:
			*scratch = crc32_end(*scratch);
#endif
		case SBMP_CKSUM_XOR:
			// scratch already contains the checksum
			break;

		case SBMP_CKSUM_NONE: // fall-through
		default:
			*scratch = 0;
			break;
	}
}

/** Check if the calculated checksum matches the received one */
bool cksum_verify(SBMP_ChecksumType type, uint32_t *scratch, uint32_t received_cksum)
{
	cksum_end(type, scratch);

	// scratch now contains the checksum

	switch (type) {

#if SBMP_HAS_CRC32
		case SBMP_CKSUM_CRC32: // fall-through
#endif
		case SBMP_CKSUM_XOR:
			return (*scratch == received_cksum);

		case SBMP_CKSUM_NONE: // fall-through
		default:
			// unknown checksum type
			return true; // assume it's OK
	}
}

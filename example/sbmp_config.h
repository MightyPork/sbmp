#ifndef SBMP_CONFIG_H
#define SBMP_CONFIG_H


/* ---------- CRC32 ---------------- */

/**
 * @brief Add support for CRC32
 *
 * Disabling CRC32 will reduce program size (for small micros).
 * If CRC32 is disabled, XOR will be used as the preferred checksum
 * method.
 *
 * Received CRC32'd messages will be accepted without checking.
 *
 * If handshake is used, the peer will detect that CRC32 is not
 * supported here, and should start using XOR.
 */
#define SBMP_HAS_CRC32 1


/* ---------- MALLOC --------------- */

/**
 * @brief Enable malloc if NULL is passed.
 *
 * This lets you malloc() the struct / buffer if you pass NULL
 * to the init functions.
 *
 * Disable malloc to free up memory taken by the malloc routine.
 * If disabled, init funcs will return NULL if NULL is passed
 * as argument.
 */
#define SBMP_USE_MALLOC 1

// those will be used if malloc is enabled
#define sbmp_malloc malloc
#define sbmp_free   free
#define sbmp_calloc calloc


/* ---------- LOGGING -------------- */

/**
 * @brief Enable logging.
 *
 * Logging functions are WEAK stubs in sbmp_logging.
 *
 * Disable logging to free up memory taken by the messages.
 */
#define SBMP_LOGGING 1

/**
 * @brief Enable detailed logging (only for debugging, disable for better performance).
 */
#define SBMP_DEBUG 1

// here are the actual logging functions
#include <stdio.h>
#define sbmp_error(fmt, ...) (SBMP_DEBUG||SBMP_LOGGING ? printf("\x1b[31;1m[SBMP][E] "fmt"\x1b[0m\n", ##__VA_ARGS__) : 0)
#define sbmp_warn(fmt, ...)  (SBMP_DEBUG||SBMP_LOGGING ? printf("\x1b[33;1m[SBMP][E] "fmt"\x1b[0m\n", ##__VA_ARGS__) : 0)
#define sbmp_info(fmt, ...)  (SBMP_DEBUG||SBMP_LOGGING ? printf("\x1b[32m[SBMP][i] "fmt"\x1b[0m\n", ##__VA_ARGS__) : 0)
#define sbmp_dbg(fmt, ...)   (SBMP_DEBUG  ? printf("[SBMP][ ] "fmt"\n", ##__VA_ARGS__) : 0)


#endif // SBMP_CONFIG_H

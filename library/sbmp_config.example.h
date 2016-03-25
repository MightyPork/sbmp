#ifndef SBMP_CONFIG_H
#define SBMP_CONFIG_H

/* --- Configuration ------------------- */


#ifndef SBMP_LOGGING
/**
 * @brief Enable logging.
 *
 * Logging functions are WEAK stubs in sbmp_logging.
 *
 * Disable logging to free up memory taken by the messages.
 */
#define SBMP_LOGGING 1
#endif


#ifndef SBMP_USE_MALLOC
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
#endif


#ifndef SBMP_HAS_CRC32
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
#endif


/* ---------- MALLOC --------------- */

#ifndef sbmp_malloc
# define sbmp_malloc malloc
#endif

#ifndef sbmp_free
# define sbmp_free free
#endif

#ifndef sbmp_calloc
# define sbmp_calloc calloc
#endif

/* ------------------------------------- */


#endif // SBMP_CONFIG_H

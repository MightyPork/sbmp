#ifndef SBMP_H
#define SBMP_H

/**
 * This is the main SBMP header file.
 *
 * This is the only header file you should need to
 * #include in your application code.
 */


/* --- Configuration ------------------- */

/**
 * @brief Enable logging.
 *
 * Logging functions are WEAK stubs in sbmp_logging.
 *
 * Disable logging to free up memory taken by the messages.
 */
#define SBMP_LOGGING 1

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
#define SBMP_MALLOC 1

/**
 * @brief Add support for CRC32
 *
 * Disabling CRC32 will reduce program size (for small micros).
 *
 * @attention
 * If CRC32 is disabled, do not build & link `crc32.o`!
 */
#define SBMP_SUPPORT_CRC32 1

/* ------------------------------------- */


// Common utils & the frame parser
#include "sbmp_logging.h"
#include "sbmp_frame.h"

// Datagram & session layer
#include "sbmp_datagram.h"
#include "sbmp_session.h"

#endif /* SBMP_H */

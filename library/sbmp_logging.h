#ifndef SBMP_COMMON_H
#define SBMP_COMMON_H

#include "sbmp_config.h"

/**
 * SBMP logging utils
 */


#if SBMP_LOGGING

// Define logging prototypes
// You can override those in the application code

/**
 * @brief Error print function, can be replaced by custom one.
 *
 * NO-OP if not overriden.
 *
 * @param format : printf format
 * @param ...    : format substitutions
 */
extern void __attribute__((weak, format(printf, 1, 2)))
sbmp_error(const char* format, ...);

/**
 * @brief Info message print function, can be replaced by custom one.
 *
 * NO-OP if not overriden.
 *
 * @param format : printf format
 * @param ...    : format substitutions
 */
extern void __attribute__((weak, format(printf, 1, 2)))
sbmp_info(const char* format, ...);


#else /* SBMP_LOGGING */

// do-nothing definitions
#define sbmp_error(...)
#define sbmp_info(...)

#endif /* SBMP_LOGGING */


#endif /* SBMP_COMMON_H */

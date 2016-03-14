#ifndef SBMP_COMMON_H
#define SBMP_COMMON_H

/**
 * SBMP common utilities etc.
 */

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


#endif /* SBMP_COMMON_H */

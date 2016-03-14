#ifndef SBMP_COMMON_H
#define SBMP_COMMON_H

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


// lsb, msb for uint16_t
#define U16_LSB(x) ((x) & 0xFF)
#define U16_MSB(x) ((x >> 8) & 0xFF)

#endif /* SBMP_COMMON_H */

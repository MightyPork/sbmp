#ifndef SBMP_COMMON_H
#define SBMP_COMMON_H

/**
 * @brief Error print function, can be replaced by custom one.
 *
 * @param format : printf format
 * @param ...    : format substitutions
 */
extern void __attribute__((weak, format(printf, 1, 2)))
sbmp_error(const char* format, ...);

#endif /* SBMP_COMMON_H */

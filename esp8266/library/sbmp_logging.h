#ifndef SBMP_COMMON_H
#define SBMP_COMMON_H

#include "sbmp_config.h"
#include "esp8266.h"

/**
 * SBMP logging utils
 */

// do-nothing definitions
#define sbmp_error(fmt, ...) os_printf("\x1b[31;1mSBMP][E] "fmt"\x1b[0m\r\n", ##__VA_ARGS__)
#define sbmp_info(fmt, ...) os_printf("\x1b[32;1m[SBMP][i] "fmt"\x1b[0m\r\n", ##__VA_ARGS__)

#endif /* SBMP_COMMON_H */

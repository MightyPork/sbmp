#include "sbmp_common.h"

void __attribute__((weak))
sbmp_error(const char* format, ...)
{
	(void)format; // do nothing
}

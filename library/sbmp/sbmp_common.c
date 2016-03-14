#include "sbmp_common.h"
#include <stdio.h>
#include <stdarg.h>

#if SBMP_LOGGING

void __attribute__((weak))
sbmp_error(const char* format, ...)
{
	printf("[SBMP] [E] ");
	va_list va;
	va_start(va, format);
	vprintf(format, va);
	va_end(va);
	printf("\n");
}

void __attribute__((weak))
sbmp_info(const char* format, ...)
{
	printf("[SBMP] [i] ");
	va_list va;
	va_start(va, format);
	vprintf(format, va);
	va_end(va);
	printf("\n");
}

#endif

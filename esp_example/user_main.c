/* This example is in the public domain */

// include stuff that's needed from the sdk...
#include <esp8266.h>

// user files
#include "serial.h"
#include "datalink.h"
#include "uart_driver.h"

#include "sbmp.h"


/**
 * Main routine.
 */
void user_init(void)
{
	// set up the debuging output
	serialInit();

	os_printf("\n\x1b[32;1mHello.\x1b[0m\n");

	// set up SBMP
	datalinkInit();

	os_printf("\nReady\n");
}


void user_rf_pre_init() {} // dummy func needed by some SDKs

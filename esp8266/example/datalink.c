#include "esp8266.h"

#include "uart_driver.h"
#include "sbmp.h"

#include "datalink.h"


static SBMP_Endpoint *ep;


/** func used for sending bytes by SBMP */
static void FLASH_FN u0_putc(uint8_t c)
{
	UART_WriteChar(UART0, c, 0);
}


static void FLASH_FN dg_handler(SBMP_Datagram *dg)
{
	sbmp_info("Datagram received.");
	// TODO: do something with the datagram now !
}

/** Function that is called from the serial task with incoming bytes */
void datalink_receive(uint8_t byte)
{
	sbmp_ep_receive(ep, byte);
}

/** Datalink */
void FLASH_FN datalinkInit(void)
{
	ep = sbmp_ep_init(NULL, NULL, 256, dg_handler, u0_putc);

	sbmp_ep_enable(ep, true);

	os_printf("SBMP started on UART0\n");
}

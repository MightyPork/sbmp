SBMP library, v1.4
==================

This is a reference implementation of SBMP, which should be usable in embedded
environment.

The library cosists of a **Framing layer**, **Datagram middleware** and a **Session layer**.

How to use
----------

The framing layer is isolated in the `sbmp_frame` module, and can be used on it's own,
if sessions are not needed. That gives you reliable data transfer with error detection.

If you want to *get the most out of SBMP*, use the session layer. Session layer functions are
namespaced `sbmp_ep_` ("ep" stands for endpoint).

All header files are included in `sbmp.h`, which is the only file you should
`#include` in your application.

Read comments in the examples to see how to use the library.

Configuration & porting
-----------------------

You can customize a lot of aspects of SBMP in `sbmp_config.h`.

If you included the library as a submodule, and want to avoid manual edits, you can set
the options using compiler flags (eg. to disable CRC32: `-DSBMP_HAS_CRC32=0`).

**What to change for Arduino**

Basically disable all you can in the config file.

- You don't want `malloc()`
- Logging is useless if there's only one USART anyway
- CRC32 can be replaced with XOR if you don't care about reliability that much.

This gains you a huge size reduction, and the whole library will take only around
2.5 kB flash and 150 B ram (of course, not including your Rx buffer).

It works really well for ATmega328P - the usual Arduino chip.

Example for AVR
---------------

This example uses the [AVR C boilerplate](https://github.com/MightyPork/avr-c-boilerplate).

It's here just to show how to set up SBMP in your AVR application.

**NOTE:** This example uses static allocation of the struct and buffer.

If you pass NULLs to the init function, it will `malloc()` it for you
and return a pointer to the Endpoint.

```c
#include <avr/io.h>          // register definitions
#include <avr/interrupt.h>   // interrupt vectors

#include <stdint.h>
#include <stdbool.h>

// AVR C boilerplate libs
#include "lib/iopins.h"
#include "lib/usart.h"

// SBMP
#include "sbmp/sbmp.h"

// SBMP globals
#define RXBUF_LEN 128
static uint8_t rxbuf[RXBUF_LEN];
static SBMP_Endpoint ep;

/** USART receive handler */
ISR(USART_RX_vect)
{
  // Get the byte and pass it to SBMP
  uint8_t b = usart_rx();
  sbmp_ep_receive(&ep, b); // WARN: This can fail. Should check retval.
}

/** Message received from SBMP */
void message_received(SBMP_Datagram *dg)
{
  // ...
}

int main()
{
  usart_init(BAUD_115200);
  usart_isr_rx_enable(true); // enable the interrupt

  // init SBMP
  sbmp_ep_init(&ep, rxbuf, RXBUF_LEN, message_received, usart_tx);

  // ...additional SBMP configuration if needed...

  // enable rx, tx
  sbmp_ep_enable(&ep, true);

  // globally enable interrupts (for the USART_RX handler)
  sei();

  while (1) {
    // ... do stuff
  }
}
```


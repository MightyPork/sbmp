SBMP for ESP8266
================

This is a modified version of the standard library, adjusted to 
work in the ESP8266 IoT SDK.

May be broken
-------------

This is literally ripped form one of my projects, so I apologize if 
there are some defines or prototypes missing, I've modified my SDK 
a bit.

If you find something, please let me know (submit an issue on GitHub).

How to use it
-------------

The `library/esp_includes/` folder contains some custom headers needed, mostly
taken from *libesphttpd* and the SDK. You may want to copy those to your project 
and adjust as needed.

Some example code on how to set up UART and SBMP are in the `example/` folder,
though take it more as an inspiration rather than a working example.

- `serial.c` configues the UARTs (UART0 for SBMP, UART1 to print debug
  messages with os_printf). It also handles the Rx interrupts.

- `datalink.c` configures SBMP.

If you get compile errors, and figure them out, please let me know so I can
add a note here.



# Simple Binary Messaging Protocol

> **This protocol is obsolete!**
>
> A better implementation - cleaner, less convoluted 
> and more configurable is now available at [MightyPork/TinyFrame](https://github.com/MightyPork/TinyFrame)!

## Introduction

SBMP is an extensible, versatile protocol for point-to-point communication
between two embedded controllers or controller and PC, based on serial port.

SBMP can handle multiple request-response sessions at the same time thanks to
a built-in message chaining schema.


### Possible applications

SBMP provides a simple way to build binary API for PC-device or device-device 
communication.

- Controlling a USART-enabled module, such as the ESP8266, by external processor
- Data exchange between two embedded controllers, such as a master controller 
  and a display driver.
- Simple API for controlling and reading data from an acquisition module (eg. 
  ARM or AVR - "Arduino" - based). This allows to build a variety of PC 
  front-ends with a standartised binary interface.

### Is SBMP for you?

If you want any of the following, SBMP may be for you:

- Reliable point-to-point communication, with CRC32 or XOR checksums
- Interleaved sessions (ask for A, receive some status updates, ask for B, receive A, receive B) - without losing track of what's a reply to what.
- Extensible - put whatever data you want in the message payload. SBMP works like a wrapper.
- Portable - the same library code can run on PC, Arduino, STM32... - no worries about compatibility

However, it's **not good for everything**, eg if you want:

- Low overhead USART communication - SBMP has some overhead, ie. checksums, session ID...
- Very short messages - using a special protocol may be overkill
- Minimal memory footprint - SBMP takes roughly 2.5 kB of flash on AVR, after disabling CRC32, debugging and malloc. This is a concern only if you're really constrained.

## Specification

The current specification (draft) is in the [spec](spec/) folder.

## Library

A reference implementation of SBMP is in the [library](library/) folder.

**NOTE** The ESP8266 variant is on the `esp8266` branch, which allows for easy updating from this master version.
Switch to that branch to get the code. The example and additional information for that branch is in the `esp_*` folders,
and the `README_ESP.md` file in the library folder.

The library works on AVR (ATmega328p) and should also work on STM32
or any other microcontroller. Naturally it works also on desktop, 
if you build it with GCC.



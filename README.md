# Simple Binary Messaging Protocol

<i>
Simple Binary Messaging Protocol specification <br>
rev. 1.0, 11 March 2016
</i>


## Introduction

SBMP is an extensible, versatile protocol for point-to-point communication
between two embedded controllers, based on USART / serial port.

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


## Conventions

- The protocol uses little-endian encoding for multi-byte numbers (LSB first)
- All bytes have 8 bits.


## Physical & framing layer

The protocol is designed for serial interfaces (USART based), such as RS232.

Naturally it can be used for inter-MPU communication using the built-in USART
peripherals.

- [Framing layer for USART](FRAMING_LAYER.md)


## Session layer

SBMP uses a system of numbered sessions (or "transactions") to ensure each 
message can be put in the right context, even if other messages were sent
before receiving a reply.

- [Session specification](SESSION_LAYER.md)


## Datagram types and structure

TBD

*End of file*



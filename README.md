# Simple Binary Messaging Protocol

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

## Specification

The current specification (draft) is in the [spec](spec/) folder.

## Library

A complete library is not yet finished, but the work-in-progress code
can be found in the [library](library/) folder.


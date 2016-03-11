# Simple Binary Messaging Protocol

## Introduction

SBMP is an extensible, versatile protocol for point-to-point communication
between two embedded controllers.

The protocol is designed for use on USART-based communication channels.

SBMP can be used in half-duplex or full duplex mode, a built-in message chaining schema
ensures message context can always be easily identified. However, in order to keep resource
usage at minimum, half-duplex mode is recommended.

### Possible applications

SBMP provides a simple way to build binary API for PC-device or device-device communication.

- Controlling a USART-enabled module, such as the ESP8266, by external processor
- Data exchange between two embedded controllers, such as a master controller and a display driver.
- Simple API for controlling and reading data from an acquisition module (eg. ARM or AVR ["Arduino"] based).
  This allows to build a variety of PC front-ends with a standartised binary interface.

## Protocol specification

### Physical layer

SBMP is designed for use on USART-based interfaces, such as simple UART or USART (available as 
on-chip peripherals of many embedded controllers), RS232 and similar.

The protocol supports only point-to-point mode, partly due to limitations of the USART interface.

The standard USART mode used is *115200 baud, 8 bits, 1 stop bit, no parity*.

### Packet structure

The SBMP communication consists of binary data packets.

Basic packet structure is as follows:

```none
+----------+------------+--------------+----------------------+----------+
|   0x01   | session ID | payload type |       payload        |   0x04   |
| (1 byte) |  (1 byte)  |   (1 byte)   | (determined by type) | (1 byte) |
+----------+------------+--------------+----------------------+----------+
```

#### Start and stop byte

Message begins with a start byte, and is terminated by a stop byte.

This allows to use the same data link for debug messages and data packets at the same time.

Debug messages will be visible in terminal connected to the data line, and the receiving party
will safely ignore them (provided they do not contain the control character `0x01`).

The data packets will be shown as gibberish in the terminal, but if only one USART interface
is available, it's a good compromise.

#### Session ID

Each packet starting a session contains an ID number, which is subsequently re-used by the 
receiving party as an ID in the response packet.

This makes it easy for the requesting party to identify what request the response relates to.

In multi-request sessions (such as a bulk data transfer with flow control - "chunking"), the same
ID is maintained thorough the session.

The two parties are identified by a "origin" bit:

```none
+------------+--------------+
| origin bit | ID 0x00-0x7F |
+------------+--------------+
```

#### Origin bit arbitration

Before a party can 


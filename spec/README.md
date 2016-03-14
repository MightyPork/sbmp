# Simple Binary Messaging Protocol

<i>
Simple Binary Messaging Protocol specification <br>
rev. 1.2, 14 March 2016
</i>

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



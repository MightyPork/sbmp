# SBMP framing layer for USART

<i>
Simple Binary Messaging Protocol specification <br>
rev. 1.2, 14 March 2016
</i>

SBMP is designed for use on USART-based interfaces, such as simple TTL U(S)ART
(available as on-chip peripherals of many embedded controllers), RS232 and
similar.

This document describes a SBMP framing layer to be used with USART-based
interfaces.

The standard configuration is *115200 baud, 8 bits, 1 stop bit, no parity*.


## Packet structure

Each message is contained in a binary packet (frame) of the following structure:

```none
+-------+-----------------------+----------------+------------+---------+------------------+
| Start | Payload checksum type | Payload length | Header XOR | Payload | Payload checksum |
| 0x01  | 1 byte                | 2 bytes        | 1 byte     |         | (0 or 4 B)       |
+-------+-----------------------+----------------+------------+---------+------------------+
```

The `Header XOR` field contains XOR of the preceding 4 bytes. It's used to check
integrity of the header. If the `Header XOR` does not match, the receiver MUST
discard the packet.

The payload checksum type is specified in in the header, so the checksum can be
calculated on-the-fly by the receiving party.

*If the checksum type is 0, the checksum field is omitted.*


## Checksum types

Checksum type codes:

- 0 - no checksum. *The checksum field is omitted.*
- 32 - CRC32 (ANSI)

Numbers 100-255 can be used for custom checksum types. Numbers < 100 are
reserved for possible use in future versions of this specification.

**If possible, CRC32 should be used.**

If a receiver does not support the checksum type used, it should simply ignore
it.

*End of file.*


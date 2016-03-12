# SBMP framing layer for USART

<i>
Simple Binary Messaging Protocol specification <br>
rev. 1.1, 12 March 2016
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
+-------+---------------+----------------+------------+---------+------------+
| Start | Checksum type | Payload length | Header XOR | Payload | Checksum   |
| 0x01  | 1 byte        | 2 bytes        | 1 byte     |         | (0 or 4 B) |
+-------+---------------+----------------+------------+---------+------------+
```

The Header XOR field contains XOR of the preceding 4 bytes. It's used to check
integrity of the header - a simple and cheap parity check.

If the XOR does not match, the receiver can be confident it's not a valid 
packet, and MUST discard it.

The checksum type is specified in advance, so the checksum can be calculated
"on-the-fly" by the receiver.

*If the checksum type is 0, the checksum field is omitted.*


## Checksum types

Currently supported checksum types:

- 0 - no checksum. *The checksum field is omitted.*
- 8 - CRC8 One-wire, generating polynomial: x^8 + x^5 + x^4 + 1
- 16 - CRC16-IBM, generating polynomial:  x^16 + x^15 + x^2 + 1
- 32 - CRC32 (ANSI)

**If possible, CRC32 should be used.**

Other types are intended for applications where certain type of CRC is already
implemented for different purpose, and can be re-used.

If an unknown checksum type is used, but the header XOR matches, the receiver 
may choose to accept the packet and ignore the checksum field.


## Shared serial line for SBMP and ASCII debug messages

Each message must begin with a start byte `0x01`.

This allows -- if needed -- to dump ASCII debug messages to the same serial
line, without disturbing the communication (provided the messages don't contain
the control character `0x01` and are printed only when the line is idle).

Such messages can then be viewed by any terminal connected to the data line.

This is *not a recommended practice*, but may be used for quick debugging when 
only one USART peripheral is available.

*End of file.*


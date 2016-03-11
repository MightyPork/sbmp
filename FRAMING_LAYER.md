# SBMP framing layer

This document describes a SBMP framing layer to be used with USART-based interfaces.

## Packet structure

Each message is contained in a binary packet (frame) of the following structure:

```none
+------------+-----------------+-------------+-----------------+------------------+
| Start byte | Payload length  |   Payload   |  Checksum type  |     Checksum     |
|    0x01    |    2 bytes      |             |     1 byte      |  (0 or 4 bytes)  |
+------------+-----------------+-------------+-----------------+------------------+
```

The length and checksum fields are little-endian (LSB first).

*If the checksum type is `0`, the checksum field is omitted.*

### Checksum types

The current checksum types are as follows:

- 0 - no checksum. *The checksum field is omitted.*
- 8 - CRC8 One-wire `x^8 + x^5 + x^4 + 1`
- 16 - CRC16-IBM `x^16 + x^15 + x^2 + 1`
- 32 - CRC32 (ANSI)

**If possible, CRC32 should be used.** 

Other types are intended for applications where certain type of CRC is already
implemented for different purpose, and can be re-used.

If an invalid or unknown checksum type is used, the receiver MUST discard the packet.
Such condition is treated as a framing error.

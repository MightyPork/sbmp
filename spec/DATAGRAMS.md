# SBMP datagram types and structure

<i>
Simple Binary Messaging Protocol specification <br>
rev. 1.2, 14 March 2016
</i>

## Datagram types

### Handshake datagrams

| Dg. type | Meaning
| -------: | :------
| 0x00     | Handshake request
| 0x01     | Handshake confirmation (origin request accepted)
| 0x02     | Handshake conflict

The handshake payload structure is specified in [SESSION_LAYER.md](SESSION_LAYER.md):

```none
preferred_checksum_type : 1 byte
rx_buffer_size          : 2 bytes
```

### User datagrams

Ohter numbers are free to be used by the application.

To avoid possible conflict with a future version of this spec, it's recommended to preferably
use numbers >= 100 for user payloads.


## Data encoding

The protocol is little-endian, LSB is always sent first:

```none
+-----+-----+
| LSB | MSB |
+-----+-----+
```

*Follow some non-normative suggestions*

**Strings** can be encoded either as null-terminated C strings, or length-prefixed.

With length-prefixed strings, the string length is sent first as `uint16_t`, followed by a string
*without* the null terminator. It's possible to use `uint8_t` lengths, if it makes sense for the
application - the encoding is by definition application-specific, choose what fits you best.

**Arrays** can be encoded in a similar way to length-prefixed strings.

It is also possible to send arbitrary **structs** using the protocol, but care must be taken so that
both parties use the same struct padding and packing options.

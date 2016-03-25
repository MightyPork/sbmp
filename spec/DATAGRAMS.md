# SBMP datagram types and structure

<i>
Simple Binary Messaging Protocol specification <br>
rev. 1.4, 26 March 2016
</i>

## Datagram types

### Handshake datagrams

| Datagram type | Description
| ------------- | -----------
| 0x00          | Handshake request
| 0x01          | Handshake confirmation (origin request accepted)
| 0x02          | Handshake conflict

The handshake payload structure is specified in
[SESSION_LAYER.md](SESSION_LAYER.md).

Both request and confirmation datagrams have the same structure, the conflict
datagram has no data payload.

```none
 Handshake payload
+-------------------------+--------------------+
| Preferred checksum type | rx_buffer_size 0:1 |
+-------------------------+--------------------+
```

See the session layer spec for more details.

All messages in a handshake must have the same session number, otherwise the
handshake will fail.

**The handshake functionality is built into the reference library, you don't
need to implement it in your code. There are functions in the session layer
to control the handshake.**


### Chunked Bulk Transfer

*Chunked bulk transfer* is a way of sending large amount of data in a series of
short data packets (size is limited by the rx buffers).

It's recommended to use the "session listener" feature of the reference library
(if you use the library).

| Datagram type | Description
| ------------- | -----------
| 0x04          | Bulk transfer offer
| 0x05          | Bulk transfer data request
| 0x06          | Bulk transfer data payload
| 0x07          | Bulk transfer abort


#### 0x04 - Bulk transfer offer

This datagram's payload starts with the total data length in bytes, as a 4-byte
(32-bit) unsigned integer.

The rest of the datagram is unspecified and can be used for user data
(implementation specific).

```none
+------------+- - - - - - -+
| Length 0:3 |  user data  |
+------------+- - - - - - -+
```

The *Bulk transfer offer* packet can be sent as a response to a request (user
datagram), or to announce that data is ready (so the receiver knows it can
read it if needed). In the latter case, the "user data" field can be used
to describe what kind of data is available.


#### 0x05 - Bulk transfer data request

This datagram is a response to the 0x04 (*Bulk transfer offer*), and must have
the same session number, so the peer knows what data is requested.

The payload consists of a data offset, and a chunk size.

If the peer does not support seeking, and the offset is discontinuous
(ie., offset 0 is requested after offset 100 has already been read), then the
peer should abort the transfer using 0x07 (*Bulk transfer abort*).

```none
+------------+----------------+
| Offset 0:3 | Chunk size 0:1 |
+------------+----------------+
note: '0:3' indicates a 4-byte number in little endian
```


#### 0x06 - Bulk transfer data payload

This datagram is a response to 0x05 (*Bulk transfer data request*).

The payload is simply the chunk of data requested.

If the requested chunk size exceeds the available data length, only the
available part will be sent.

The requesting party can easily check the payload length from the frame
header.

```none
+- - - - - -+
|  Payload  |
+- - - - - -+
```


#### 0x07 - Bulk transfer abort

This datagram aborts an ongoing bulk transfer.

It can be sent by the requesting party to discard rest of the data, or by the
responding party if the data is no longer available.

Examples when this datagram is used:

- The requesting party doesn't want to read any more of the data.
- The data is no longer available.
- The requested offset is out of range.
- The requested offset cannot be served (ie. the data is read from hardware when
  requested, and it's not possible to read the same data again)

This datagram has no data payload.


### User datagrams

Other numbers are free to be used by the application.

To avoid possible conflict with a future version of this spec, it's recommended to preferably
use numbers >= 100 for user payloads.


## Data encoding

The protocol is little-endian, LSB is always sent first:

```none
+---------+
| LSB:MSB |
+---------+
```

---

*The following are some non-normative suggestions*

**Strings** can be encoded either as null-terminated C strings, or length-prefixed.

With length-prefixed strings, the string length is sent first as `uint16_t`, followed by a string
*without* the null terminator. It's possible to use `uint8_t` lengths, if it makes sense for the
application - the encoding is by definition application-specific, choose what fits you best.

**Arrays** can be encoded in a similar way to length-prefixed strings.

It is also possible to send arbitrary **structs** using the protocol, but care must be taken so that
both parties use the same struct padding and packing options.

*End of file.*

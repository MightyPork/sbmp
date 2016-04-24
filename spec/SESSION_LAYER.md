# SBMP session layer

<i>
Simple Binary Messaging Protocol specification <br>
rev. 1.5, 24 April 2016
</i>

The messaging protocol uses a simple session system, which allows to perform
multiple request-response transactions simultaneously without losing the
context.

## Datagram structure

Each SBMP datagram starts with a session number, followed by a datagram type
code, and a block of data.

```
+----------------+---------------+------------------+
| Session number | Datagram type | Datagram payload |
| 2 bytes        | 1 byte        | (up to ~ 64kB)   |
+----------------+---------------+------------------+
```

### Datagram type

The datagram type field is used to identify different kinds of payloads.

During the origin arbitration process (see below), it is also used as a status
field for no-payload datagrams.

| Dg. type | Meaning
| -------: | :------
| 0x00     | Handshake request
| 0x01     | Handshake confirmation (origin request accepted)
| 0x02     | Handshake conflict

Other datagram types can be used for user payloads.

It's recommended to use types >= 100, to avoid possible conflicts with a future
version of the spec.


### Payload

The datagram payload can be up to *65535-3 bytes long*.

In practice it's recommended to use shorter datagrams with flow control
(chunked transfer) if large amounts of data need to be sent.

This avoids the need for large buffers.


### Session number

The session number is used to maintain the request-response chain.

**A response always uses the same session number as the request it replies to.**

In sessions that span multiple request-response cycles, the same number can be
re-used for all requests to maintain the session context.

Two unrelated requests should never use the same session number.


## Avoiding a session number collision

The highest bit of the session number is reserved to identify which party
started the session.

This is how you'd typically obtain a session number from the origin
bit and a session counter:

```c
uint16_t session = (origin << 15) | (counter & 0x7FFF);
```

This ensures the two parties can never start a session with the same number,
if their origin bit differs.


### Handshake - origin arbitration

Hard-coding the origin bit could work for simple scenarios, but to achieve
better inter-operability, a handshake protocol is used:

Before *(our)* party can start talking on the bus, it must claim it's origin number.

This is done by sending a datagram of type `0x00` (Handshake request), with the
origin bit set to the value we are requesting.

The datagram body should contain information describing our endpoint.

For example, to request the origin bit `1`, the following datagram is used:

```none
Request to claim the origin bit "1",
- want CRC32,
- rx buffer 100 bytes long

    Session     Datagram   Preferred       Rx buffer
    number      type       checksum type   size (100 B)
+------+------+----------+---------------+------+------+
| 0x00 | 0x80 |   0x00   |      32       | 0x64 | 0x00 |
+------+------+----------+---------------+------+------+
  Session 0:1     1 byte      1 byte         Size 0:1

Please note that the S.N. is litte-endian.
```

The checksum type and buffer size fields are optional and can be left out if needed.

Those extra fields are used by the peer to tailor it's outgoing messages for us.

The receiving party replies with the same S.N., and the status in the datagram
type field:

- 0x01 (acknowledge)
- 0x02 (conflict)

The reply should also contain the extra fields.


```none
Response - "acknowledge"
- don't watch checksums
- rx buffer is 32 bytes long

  S.N.          Dg.t.   No checksums    32B buffer
+------+------+------+---------------+------+------+
| 0x00 | 0x80 | 0x01 |       0       | 0x20 | 0x00 |
+------+------+------+---------------+------+------+
  Session 0:1  1 byte    1 byte         Size 0:1
```

- If the receiving party has acknowledged *our* origin bit, it accepts the
  opposite bit, and the communication can continue.
- The *conflict* response is issued in case the receiving party has sent a
  request too, and awaits a response.

In the conflict situation, both parties discard any buffered messages and wait
for a random amount of time before retrying the attempt.

After either of the parties successfully claims the origin bit it desires,
the waiting is aborted and communication can continue.

#### Example 1 - a successful request

- Node A requests origin 1
- Node B acknowledges, claims origin 0
- *arbitration done*

This is always the case if node B is a slave. (i.e. - does not start
communication on it's own).

#### Example 2 - conflict

- Node A requests origin 1
- Node B requests origin 1 at the same time
- Node A (or B) detects the collision and replies with code 0x02 - conflict
- If the other party has already send any other response, it's discarded.
- Both parties clear their message queue and wait for a random number of
  milliseconds before performing another attempt.

*End of file.*

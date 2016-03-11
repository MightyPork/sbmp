# DBMP session layer

<i>
Simple Binary Messaging Protocol specification <br>
rev. 1.0, 11 March 2016
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
| 0x00     | Start of the origin arbitration
| 0x01     | Acknowledgement (origin request accepted)
| 0x02     | Negative acknowledgement (origin denied)
| 0x03     | Conflict in origin arbitration

Other datagram types are described in the payload structure spec. **TODO link**

The codes `0x01` and `0x02` can be freely used outside the arbitration process
with the meaning "OK" and "Not Ready" / "Rejected".

### Payload

The datagram payload can be up to *65532 bytes long* - a limitation of the 
framing layer. 

In practice however, it's recommended to use shorter datagrams with flow control
(chunked bulk transfer) if large amounts of data need to be sent.

The payload length and structure depends on the *Datagram type*.


### Session number

The session number is used to maintain the request-response chain.

**A response always uses the same session number as the request it replies to.**

In sessions that span multiple request-response cycles, the same number can be
re-used for all requests to maintain the session context.

Two unrelated requests should never use the same session number.


## Avoiding a session number collision

The highest bit of the session number is reserved to identify which party
started the session.

```none
+---------------------------------+
| Session number field            |
+------------+--------------------+
| origin bit | S.N. 0x0000-0x7FFF |
+------------+--------------------+
```

This ensures the two parties can never start a session with the same number.


### Origin arbitration

Hard-coding the origin bit could work for simple scenarios, but to achieve
better inter-operability, the session layer contains an arbitration mechanism:

Before *(our)* party can start talking on the bus, it must claim an origin number.

This is done by sending a datagram with session number `0x0000`, the origin bit
set to the value we are requesting, and datagram type set to `0x00`.

For example, to request the origin bit `1`, the following datagram is used:

```none
Request to claim the origin bit "1"

  S.N.          Dg.t.
+------+------+------+
| 0x00 | 0x80 | 0x00 |
+------+------+------+

Please note that the S.N. is litte-endian.
```

The receiving party replies with the same S.N. and a status code in the datagram
type field:

- 0x01 (acknowledge)
- 0x02 (negative acknowledge)
- 0x03 (conflict)


```none
Response - "acknowledge"

  S.N.          Dg.t.
+------+------+------+
| 0x00 | 0x80 | 0x01 |
+------+------+------+
```

- If the receiving party has acknowledged *our* origin bit, it accepts the 
  opposite bit, and the communication can continue.
- If the request was denied, we have to use the opposite bit.
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

#### Example 2 - collision with a conflict

- Node A requests origin 1
- Node B requests origin 1 at the same time
- Node A (or B) detects the collision and replies with code 0x03 - conflict
- If the other party has already send any other response, it's discarded.
- Both parties clear their message queue and wait for a random number of 
  milliseconds before performing another attempt.

#### Example 3 - collision without a conflict

- Node A requests origin 1
- Node B requests origin 0
- Node A (or B) detects the collision, but since the IDs are different,
  no conflict has occured, and the request is accepted by both sides.
- *arbitration done*

*End of file.*


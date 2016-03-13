SBMP library
============

This is an implementation of SBMP, without `malloc()`. It's more robust than the 
naive malloc variant.

How to use
----------

The framing layer is isolated in the `sbmp_frame` module, and can be used on it's own.

If you want to use the datagram layer, use `sbmp_datagram`.

All header files are included in `sbmp.h`, which is the only file you should 
`#include` in your application.

Read comments in `main.c` for an example how to use the library.

`main.c` contains a loopback example, where the "hardware" is left out and
transmitted bytes are directly sent to the receive function. This tests that both 
parts are working properly.

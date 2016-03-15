SBMP library
============

This is an example implementation of SBMP, which should be usable in embedded
environment.

The library cosists of a **Framing layer**, **Datagram middleware** and a **Session layer**.

How to use
----------

The framing layer is isolated in the `sbmp_frame` module, and can be used on it's own.

If you want to get the most out of SBMP, use the session layer. Session layer functions are 
namespaced `sbmp_ep_` ("ep" stands for endpoint).

All header files are included in `sbmp.h`, which is the only file you should 
`#include` in your application.

Read comments in `main.c` for an example how to use the library.

Note on configuration & porting
-------------------------------

You can customize a lot of aspects of SBMP in `sbmp_config.h`.

If you included the library as a submodule, and want to avoid manual edits, you can set
the options using compiler flags (eg. to disable CRC32: `-DSBMP_HAS_CRC32=0`).
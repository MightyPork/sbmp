/**
 * This is an example of using the payload parser.
 *
 * This example is in the public domain.
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "sbmp/payload_parser.h"

int main(void)
{
	uint8_t buf[] = {0x00, 0x80, 0x77, 'b', 'a', 'n', 'a', 'n', 'a', 0};

	// pp_start is a static initializer, no malloc - safe.
	PayloadParser p = pp_start(buf, 8);

	// read some numbers
	printf("%x\n", pp_u16(&p));
	printf("%x\n", pp_u8(&p));

	// get the remainder

	// our string ends with a zero, se we can ignore the length.
	// normally length is writen to a variable passed as the second argument
	printf("%s\n", pp_rest(&p, NULL));

	return 0;
}

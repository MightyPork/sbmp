#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "sbmp.h"

static SBMP_State *sbmp;


static void msg_handler(uint8_t *payload, uint16_t length)
{
	printf("Received a payload of length %u\n", length);
	for (int i = 0; i < length; i++) {
		printf("%d : %02x\n", i, payload[i]);
	}

	free(payload);
}


int main(void)
{
	sbmp = sbmp_init(msg_handler, 20);

	const uint8_t packet[] = {
		0x01,                    // start byte
		32,                      // crc type   CRC32
		0x05, 0x00,              // length     5
		0x01 ^ 32 ^ 0x05 ^ 0x00, // header XOR
		'A', 'B', 'C', 'D', 'E', // payload    ABCDE
		0xd5, 0x1a, 0xd3, 0x72,  // checksum   72d31ad5
	};

	for (size_t i = 0; i < sizeof(packet); i++) {
		sbmp_receive(sbmp, packet[i]);
	}

	// herp derp

	sbmp_destroy(sbmp);
}

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "sbmp.h"

static SBMP_State *sbmp;

static void print_char(uint8_t byte)
{
	printf("0x%02x, %d", byte, byte);
	if (byte >= 32 && byte < 128) printf(" '%c'", byte);
	printf("\n");
}


static void frame_handler(uint8_t *payload, size_t length)
{
	printf("Received a payload of length %u\n", length);
	for (size_t i = 0; i < length; i++) {
		printf("Rx : ", (int)i);
		print_char(payload[i]);
	}

	free(payload);
}

static void tx_fn(uint8_t byte)
{
	printf("Tx : ");
	print_char(byte);

	// simulate data corruption
//	if (byte == 0x4f) byte++;

	sbmp_receive(sbmp, byte);
}


int main(void)
{
	printf("\n");

	sbmp = sbmp_init(frame_handler, tx_fn, 20);

	// manually composed message

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

	printf("\n");

	// send & receive (loopback)

	sbmp_transmit_start(sbmp, 32, 11);
	sbmp_transmit_buffer(sbmp, (uint8_t *) "HELLO WORLDasdf", 15);

	// herp derp

	sbmp_destroy(sbmp);

	printf("\n");
}

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "sbmp.h"

static SBMP_State *sbmp;


// for debugging
static bool anticipate_datagram = false;

static void print_char(uint8_t byte)
{
	printf("0x%02x, %d", byte, byte);
	if (byte >= 32 && byte < 128) printf(" '%c'", byte);
	printf("\n");
}


static void frame_handler(uint8_t *payload, size_t length)
{
	printf("Received a payload of length %u\n", length);

	if (anticipate_datagram) {
		SBMP_Datagram *dg = sbmp_parse_datagram(payload, length);
		printf("Received datagram type %d, session %d, length %d\n", dg->type, dg->session, dg->length);

		for (size_t i = 0; i < dg->length; i++) {
			printf("Rx : ");
			print_char(dg->payload[i]);
		}

		sbmp_destroy_datagram(dg);
	} else {
		for (size_t i = 0; i < length; i++) {
			printf("Rx : ");
			print_char(payload[i]);
		}

		free(payload);
	}
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

	// send & receive (loopback)

	sbmp_transmit_start(sbmp, 32, 11);
	sbmp_transmit_buffer(sbmp, (uint8_t*)"HELLO WORLDasdf", 15);

	printf("\n");

	anticipate_datagram = true;

	const int len = 10;
	const int type = 100;
	const int sess = 0;
	sbmp_datagram_start(sbmp, 32, sess, type, len);
	sbmp_datagram_add_buffer(sbmp, (uint8_t*)"0123456789", len);

	sbmp_destroy(sbmp);

	printf("\n");
}

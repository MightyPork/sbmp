#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "sbmp/sbmp.h"

static void frame_handler(uint8_t *payload, uint16_t length);
static void print_char(uint8_t byte);
static void tx_fn(uint8_t byte);

// SBMP instance
static uint8_t sbmp_buf[100];
static SBMP_State sbmp;


/** To test the DG decoder func - set in main() before sending a datagram frame */
static bool anticipate_datagram = false;


int main(void)
{
	printf("\n");

	// --- Set up SBMP ---

	sbmp_init(&sbmp, sbmp_buf, 100, frame_handler, tx_fn);


	// --- Using the framing layer directly ---

	// Start a frame
	sbmp_start_frame(&sbmp, 32, 11);
	// Send frame data
	uint16_t sent = sbmp_send_buffer(&sbmp, (uint8_t*)"HELLO WORLDasdf", 15);
	// (will print only what fits in the frame)
	printf("Sent %d bytes.\n", sent);


	printf("\n");


	// --- Try sending & receiving a datagram --

	anticipate_datagram = true;

	// Start a frame + datagram
	const int len = 10;
	const int type = 100;
	const int sess = 0;
	sbmp_start_datagram(&sbmp, 32, sess, type, len);
	// Send datagram payload
	sbmp_send_buffer(&sbmp, (uint8_t*)"0123456789", len);



	printf("\n");
}


/** SBMP transmit callback */
static void tx_fn(uint8_t byte)
{
	// SBMP wants to send a byte to the USART

	printf("Tx : ");
	print_char(byte);

	// simulate data corruption
/*
	if (byte == 0x4f) {
		printf("Corrupting byte %d\n", byte);
		byte++;
	}
*/

	sbmp_receive(&sbmp, byte);
}


/** Error print function - overriding weak stub from SBMP */
void sbmp_error(const char* format, ...)
{
	// print a tag
	printf("[SBMP] ");

	// printf the message
	va_list va;
	va_start(va, format);
	vprintf(format, va);
	va_end(va);

	// newline
	printf("\n");
}


/** Print a byte hexdump-style */
static void print_char(uint8_t byte)
{
	printf("0x%02x, %d", byte, byte); // show hex and dec
	if (byte >= 32 && byte < 128) printf(" '%c'", byte); // show the char
	printf("\n");
}


/** Frame RX callback for SBMP */
static void frame_handler(uint8_t *payload, uint16_t length)
{
	printf("Received a payload of length %u\n", length);

	if (anticipate_datagram) {

		// main sent a datagram - try to parse it

		SBMP_Datagram dg;

		sbmp_parse_datagram(&dg, payload, length);

		printf("Received datagram type %d, session %d, length %d\n", dg.type, dg.session, dg.length);

		for (size_t i = 0; i < dg.length; i++) {
			printf("Rx : ");
			print_char(dg.payload[i]);
		}

	} else {

		// main used the framing layer directly

		for (size_t i = 0; i < length; i++) {
			printf("Rx : ");
			print_char(payload[i]);
		}
	}
}

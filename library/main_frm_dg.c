/**
 * This is an example of using the framing and datagram layer of SBMP.
 *
 * The framing layer can be used on it's own to ensure data integrity.
 *
 * The datagram layer on it's own is pretty useless, to get all
 * the benefits, use the session layer on top of it.
 */

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "sbmp/sbmp.h"

static void frame_received(uint8_t *payload, uint16_t length, void *token);
static void print_char(uint8_t byte);
static void usart_tx_byte(uint8_t byte);

// SBMP instance
static uint8_t sbmp_buf[100];
static SBMP_FrmState sbmp;


/** To test the DG decoder func - set in main() before sending a datagram frame */
static bool anticipate_datagram = false;


int main(void)
{
	printf("\n");

	// --- Set up SBMP ---

	sbmp_frm_init(&sbmp, sbmp_buf, 100, frame_received, usart_tx_byte);
	// Note: if the struct and/or buffer were NULL, they'd be allocated
	// In that case, a pointer to the state struct is returned.


	// --- Using the framing layer directly ---

	// Start a frame
	sbmp_start_frame(&sbmp, 32, 11);
	// Send frame data
	uint16_t sent = sbmp_send_buffer(&sbmp, (uint8_t*)"HELLO WORLDasdf", 15);
	// (will send only what fits in the frame)
	printf("Sent %d bytes.\n", sent);


	printf("\n");


	// --- Try sending & receiving a datagram --

	for (int i = 0; i < 3; i++) {
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
}


/** SBMP transmit callback */
static void usart_tx_byte(uint8_t byte)
{
	// SBMP wants to send a byte to the USART

	// In real application, we should now send it to the USART peripheral.

	printf("Tx : ");
	print_char(byte);

	// simulate data corruption
/*
	if (byte == 0x4f) {
		printf("Corrupting byte %d\n", byte);
		byte++;
	}
*/

	// Send it back - as if we just received it.
	SBMP_RxStatus status = sbmp_frm_receive(&sbmp, byte);

	// TODO should check if the byte was accepted

	// The byte may be rejected if it's invalid, or if
	// SBMP is currently busy waiting for the previous
	// frame to be processed. A retry loop with a timeout
	// should be used.

	// In real application, the sbmp_receive() function
	// will be called from an interrupt routine.
}


/** Error print function [ overriding weak stub from SBMP ] */
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


/** Frame RX callback for SBMP */
static void frame_received(uint8_t *payload, uint16_t length, void *user_token)
{
	(void)user_token; // ignore

	printf("Received a payload of length %u\n", length);

	if (anticipate_datagram) {

		// main sent a datagram - try to parse it

		SBMP_Datagram dg;

		bool suc = sbmp_parse_datagram(&dg, payload, length);
		// should now check if suc is true.

		printf("Received datagram type %d, session %d, length %d\n", dg.type, dg.session, dg.length);

		for (size_t i = 0; i < dg.length; i++) {
			printf("%c", dg.payload[i]);
		}
		printf("\n");

	} else {

		// main used the framing layer directly
		for (size_t i = 0; i < length; i++) {
			printf("%c", payload[i]);
		}
		printf("\n");
	}
}



/** Print a byte hexdump-style */
static void print_char(uint8_t byte)
{
	printf("0x%02x, %d", byte, byte); // show hex and dec
	if (byte >= 32 && byte < 128) printf(" '%c'", byte); // show the char
	printf("\n");
}

/**
 * Example of using the session layer of SBMP
 *
 * This example sets up two endpoints (devices) and virtually
 * connects them to make a loop.
 *
 * Then a handshake is triggered so the nodes get their Origin Bit.
 *
 * Finally some messages are sent and received to show that the
 * session layer is working.
 *
 * This example is in the public domain.
 */

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "sbmp/sbmp.h"

static void print_char(uint8_t byte);

static void alice_rx(SBMP_Datagram *dg);
static void alice_tx(uint8_t byte);

static void bob_rx(SBMP_Datagram *dg);
static void bob_tx(uint8_t byte);

// Our two endpoints
static SBMP_Endpoint *alice;
static SBMP_Endpoint *bob;


static void alice_40_listener(SBMP_Endpoint *ep, SBMP_Datagram *dg, void **obj);

int main(void)
{
	bool suc;

	printf("\n");

	// --- Set up SBMP ---

	// For this example, we choose to let the driver allocate the needed structs

	// You can pass your own static struct and a buffer array instead of the NULLs,
	// if you don't want to use MALLOC.

	alice = sbmp_ep_init(NULL, NULL, 100, alice_rx, alice_tx);
	bob   = sbmp_ep_init(NULL, NULL, 200, bob_rx, bob_tx);

	// OPTIONAL - good for multi-message transfers.
	sbmp_ep_init_listeners(alice, NULL, 6);
	sbmp_ep_init_listeners(bob, NULL, 6);


	// --- Prepare the endpoints ---

	// You can set the initial session number (rand). OPTIONAL
//	sbmp_ep_seed_session(alice, 1000);
//	sbmp_ep_seed_session(bob, 2000);

	// each node can choose what checksum they want to receive
	// (eg. an Arduino node may leave out the CRC32 routine for size reasons)
	sbmp_ep_set_preferred_cksum(alice, SBMP_CKSUM_CRC32);
	sbmp_ep_set_preferred_cksum(bob, SBMP_CKSUM_NONE);

	// enable the endpoints
	sbmp_ep_enable(alice, true);
	sbmp_ep_enable(bob, true);


	// --- Start handshake ---

	// Alice starts a handshake
	suc = sbmp_ep_start_handshake(alice);
	if (!suc) {
		printf("Failed to start handshake.");
		return 1;
	}

	// wait for result... sbmp_ep_handshake_status(alice)

	printf("\n");

	printf("Alice's handshake status = %d\n", sbmp_ep_handshake_status(alice));
	printf("Bob's handshake status   = %d\n", sbmp_ep_handshake_status(bob));

	printf("Alice's origin = %d\n", alice->origin);
	printf("Bob's origin   = %d\n", bob->origin);


	// --- Try sending some stuff ---

	// Alice sends ["HELLO", type 100] to Bob
	uint16_t sesn;
	sbmp_ep_send_message(alice, 100, (uint8_t*)"HELLO", 5, &sesn, NULL);
	printf("Alice sent a message with S.N. %d\n", sesn);

	// Assume it was received, bob sends a reply
	char *msg = "Sup Alice, how u doin";
	sbmp_ep_send_response(bob, 113, (uint8_t*)msg, (uint16_t)strlen(msg), sesn, NULL);

	// the other way
	msg = "HELLO, ALICE!";
	sbmp_ep_send_message(bob, 100, (uint8_t*)msg, (uint16_t)strlen(msg), NULL, NULL); // NULL - we con't care what session nr it is


	// --- Try the multi-message transfer functionality ---

	// this will similate a bulk data transfer, with fictional datagram types

	// NOTE:

	// This can't work without actual hardware with some transfer delays (or some async queue)

	// If used with synchronous callbacks like in this example,
	// alice receives a reply even before she had time to register the listener

	// Work-around would be to first make the session number,
	// and send the initial message using the "reply" function.

	msg = "Hi Bob, wanna start a session?";

	if (false) {

		// this is the normal way to do it - would not work here. (see above)
		sbmp_ep_send_message(alice, 40, (uint8_t*)msg, (uint16_t)strlen(msg), &sesn, NULL);
		sbmp_ep_add_listener(alice, sesn, alice_40_listener, NULL);

	} else {
		// this is how we do it here to avoid the above mentioned limitation

		// NOTE: it still won't work right here, it's here for illustration only

		// first get the session number, and register the listener
		sesn = sbmp_ep_new_session(alice);
		sbmp_ep_add_listener(alice, sesn, alice_40_listener, NULL);

		// use the "response" function instead of "message" - they are the same apart from the session number handling
		// "message" creates a new session nr and puts it in the provided pointer, "response" accepts it as an argument.
		suc = sbmp_ep_send_response(alice, 40, (uint8_t*)msg, (uint16_t)strlen(msg), sesn, NULL);
		// the session hasn't been started, remove the listener which is now useless
		if (!suc) sbmp_ep_remove_listener(alice, sesn);
	}

	// rest is done in the listeners
	// we have the session nr in sesn

	printf("Done.\n");
}



// we will use:
// type "40" to start the multi-message transfer,
// type "41" to acknowledge
// type "42" to request data
// type "43" to reply data
// type "44" to terminate the session

static void alice_40_listener(SBMP_Endpoint *ep, SBMP_Datagram *dg, void **obj)
{
	printf("ALICE received a message in session %d - type %d\n", dg->session, dg->type);

	printf("\x1b[32m[ALICE] Received: ");
	for (int  i = 0; i < dg->length; i++) {
		printf("%c", dg->payload[i]);
	}
	printf("\x1b[0m\n");

	// if alice received 41, means bob agrees to the transfer

	if (dg->type == 41) {
		// bob agrees to the bulk transfer

		// Alice can now send 42 to request data

		// THIS WONT WORK HERE BECAUSE WE DON'T HAVE DELAYS (in this example)
		// bob is still in the callback - can't handle new response :(
	}

	// if received 43, data is received from bob
	if (dg->type == 43) {
		// bob sent us some data...

		// process & can also query for more
	}

	if (dg->type == 44) {
		// bob wants to close the connection
		// (something happened and the data is perhaps no longer available)

		// this will normally not be needed
		sbmp_ep_remove_listener(ep, dg->session);
	}
}


static void bob_40_listener(SBMP_Endpoint *ep, SBMP_Datagram *dg, void **obj)
{
	printf("BOB received a message in session %d - type %d\n", dg->session, dg->type);

	printf("\x1b[36m[BOB] Received: ");
	for (int  i = 0; i < dg->length; i++) {
		printf("%c", dg->payload[i]);
	}
	printf("\x1b[0m\n");

	// 42 - alice requests data -> reply with 43 and data

	if (dg->type == 44) {
		// alice wants to close the connection
		sbmp_ep_remove_listener(ep, dg->session);

		// do needed cleanup if any
	}
}



// --- Endpoint implementations ---

static void alice_rx(SBMP_Datagram *dg)
{
	printf("\x1b[32m[ALICE] received a datagram: sn %d, type %d, len %d\x1b[0m\n", dg->session, dg->type, dg->length);

	printf("\x1b[32m[ALICE] Received: ");
	for (int  i = 0; i < dg->length; i++) {
		printf("%c", dg->payload[i]);
	}
	printf("\x1b[0m\n");
}

static void alice_tx(uint8_t byte)
{
	printf("[A→B] ");
	print_char(byte);

	sbmp_ep_receive(bob, byte);
}



static void bob_rx(SBMP_Datagram *dg)
{
	printf("\x1b[36m[BOB] received a datagram: sn %d, type %d, len %d\x1b[0m\n", dg->session, dg->type, dg->length);

	printf("\x1b[36m[BOB] Received: ");
	for (int  i = 0; i < dg->length; i++) {
		printf("%c", dg->payload[i]);
	}
	printf("\x1b[0m\n");

	// part of the listener example:
	if (dg->type == 40) {
		// alice wants to start a session
		sbmp_ep_add_listener(bob, dg->session, bob_40_listener, NULL);

		// TODO send reply, like 41 - indicate that we acknowledge the transfer
	}
}

static void bob_tx(uint8_t byte)
{
	printf("[A←B] ");
	print_char(byte);

	sbmp_ep_receive(alice, byte);
}


// --- debug & utils ---

///** Error print function [ overriding weak stub from SBMP ] */
//void sbmp_error(const char* format, ...)
//{
//	printf("[SBMP] [E] ");
//	va_list va;
//	va_start(va, format);
//	vprintf(format, va);
//	va_end(va);
//	printf("\n");
//}

///** Info print function [ overriding weak stub from SBMP ] */
//void sbmp_info(const char* format, ...)
//{
//	printf("[SBMP] [i] ");
//	va_list va;
//	va_start(va, format);
//	vprintf(format, va);
//	va_end(va);
//	printf("\n");
//}

/** Print a byte hexdump-style */
static void print_char(uint8_t byte)
{
	printf("0x%02x, %d", byte, byte); // show hex and dec
	if (byte >= 32 && byte < 128) printf(" '%c'", byte); // show the char
	printf("\n");
}

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

int main(void)
{
	printf("\n");

	// --- Set up SBMP ---

	// For this example, we choose to let the driver allocate the needed structs
	// You can pass your own static struct and array instead of the NULLs,
	// if you don't want to use MALLOC.

	alice = sbmp_ep_init(NULL, NULL, 40, alice_rx, alice_tx);
	bob   = sbmp_ep_init(NULL, NULL, 60, bob_rx, bob_tx);

	// --- Prepare the endpoints ---

	// You can set the initial session number (rand).
	// This *could* help with debugging, and make handshake more reliable. Maybe.
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
	bool suc = sbmp_ep_start_handshake(alice);
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
	sbmp_ep_send_message(alice, 100, "HELLO", 5, &sesn, NULL);
	printf("Alice sent a message with S.N. %d\n", sesn);

	// Assume it was received, bob sends a reply
	char *msg = "Sup Alice, how u doin";
	sbmp_ep_send_response(bob, 113, msg, (uint16_t)strlen(msg), sesn, NULL);

	// the other way
	msg = "HELLO, ALICE!";
	sbmp_ep_send_message(bob, 100, msg, (uint16_t)strlen(msg), NULL, NULL); // NULL - we con't care what session nr it is

	printf("Done.\n");
}

// --- Endpoint implementations ---

static void alice_rx(SBMP_Datagram *dg)
{
	printf("\e[32m[ALICE] received a datagram: sn %d, type %d, len %d\e[0m\n", dg->session, dg->type, dg->length);

	printf("\e[32m[ALICE] Received: ");
	for (int  i = 0; i < dg->length; i++) {
		printf("%c", dg->payload[i]);
	}
	printf("\e[0m\n");
}

static void alice_tx(uint8_t byte)
{
	printf("[A→B] ");
	print_char(byte);

	sbmp_ep_receive(bob, byte);
}



static void bob_rx(SBMP_Datagram *dg)
{
	printf("\e[36m[BOB] received a datagram: sn %d, type %d, len %d\e[0m\n", dg->session, dg->type, dg->length);

	printf("\e[36m[BOB] Received: ");
	for (int  i = 0; i < dg->length; i++) {
		printf("%c", dg->payload[i]);
	}
	printf("\e[0m\n");
}

static void bob_tx(uint8_t byte)
{
	printf("[A←B] ");
	print_char(byte);

	sbmp_ep_receive(alice, byte);
}


// --- debug & utils ---

/** Error print function [ overriding weak stub from SBMP ] */
void sbmp_error(const char* format, ...)
{
	printf("[SBMP] [E] ");
	va_list va;
	va_start(va, format);
	vprintf(format, va);
	va_end(va);
	printf("\n");
}

/** Info print function [ overriding weak stub from SBMP ] */
void sbmp_info(const char* format, ...)
{
	printf("[SBMP] [i] ");
	va_list va;
	va_start(va, format);
	vprintf(format, va);
	va_end(va);
	printf("\n");
}

/** Print a byte hexdump-style */
static void print_char(uint8_t byte)
{
	printf("0x%02x, %d", byte, byte); // show hex and dec
	if (byte >= 32 && byte < 128) printf(" '%c'", byte); // show the char
	printf("\n");
}

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "project2.h"

/* ***************************************************************************
 ALTERNATING BIT AND GO-BACK-N NETWORK EMULATOR: VERSION 1.1  J.F.Kurose

 This code should be used for unidirectional or bidirectional
 data transfer protocols from A to B and B to A.
 Network properties:
 - one way network delay averages five time units (longer if there
 are other messages in the channel for GBN), but can be larger
 - packets can be corrupted (either the header or the data portion)
 or lost, according to user-defined probabilities
 - packets may be delivered out of order.

 Compile as gcc -g project2.c student2.c -o p2
 **********************************************************************/

/********* STUDENTS WRITE THE NEXT SEVEN ROUTINES *********/
/* 
 * The routines you will write are detailed below. As noted above, 
 * such procedures in real-life would be part of the operating system, 
 * and would be called by other procedures in the operating system.  
 * All these routines are in layer 4.
 */
extern int TraceLevel;

#define CORRUPT (0)
#define NOT_CORRUPT (1)

#define READY (0)
#define NOT_READY (1)

#define TIMEOUT (5000)

struct pkt universalAPkt;
int SendStatus;

struct pkt universalBPkt;
int previousSeqNum;

int flipBit(int seqnum) {
	if (seqnum == 1) {
		seqnum = 0;
	} else {
		seqnum = 1;
	}
	return seqnum;
}

int checkSum(char data[20]) {
	int total = 0;
	for (int i = 0; i < 20; i++) {
		total += (data[i] * (i + 1));
	}
}

int PacketStatus(struct pkt packet) {
	if (checkSum(packet.payload) != packet.checksum) {
		if (TraceLevel > 1) {
			printf(
					"Corrupted Packet Checksum!\n Calculated:%d\n Actual: %d;\n\n",
					checkSum(packet.payload), universalAPkt.checksum);
		}
		return CORRUPT;
	}
	if (packet.seqnum != 0 && packet.seqnum != 1) {
		if (TraceLevel > 1) {
			printf("Corrupted Packet Sequence Number!\n Calculated:%d\n Actual: %d;\n\n",
					packet.seqnum, universalAPkt.seqnum);
		}
		return CORRUPT;
	}
	if (packet.acknum != 1 && packet.acknum != 0) {
		if (TraceLevel > 1) {
			printf("Corrupted Packet ACK Number!\n Packet Acknum: %d;\n\n",
					universalAPkt.acknum);
		}
		return CORRUPT;
	}
	return NOT_CORRUPT;
}

/* Message */
struct Node {
	char data[20];
	struct Node* next;
};

// Stores first and last nodes.
struct Node* first = NULL;
struct Node* last = NULL;

void enqueue(char x[20]) {
	struct Node* temp = (struct Node*) malloc(sizeof(struct Node));

	strncpy(temp->data, x, 20);
	temp->next = NULL;

	if (first == NULL && last == NULL) {
		first = last = temp;
		return;
	}

	last->next = temp;
	last = temp;
	return;
}

char *dequeue() {
	struct Node* temp = first;
	if (first == NULL) {
		return NULL;
	}
	if (first == last) {
		first = last = NULL;
	} else {
		first = first->next;
	}
	return temp->data;
}

/* 
 * A_output(message), where message is a structure of type msg, containing 
 * data to be sent to the B-side. This routine will be called whenever the 
 * upper layer at the sending side (A) has a message to send. It is the job 
 * of your protocol to insure that the data in such a message is delivered 
 * in-order, and correctly, to the receiving side upper layer.
 */
void A_output(struct msg message) {
	if (SendStatus == NOT_READY) {
		enqueue(message.data);
		return;
	} else if (SendStatus == READY) {
		universalAPkt.seqnum = flipBit(universalAPkt.seqnum);
		universalAPkt.acknum = 0;
		strncpy(universalAPkt.payload, message.data, 20);
		universalAPkt.checksum = checkSum(universalAPkt.payload);

		stopTimer(0);
		startTimer(0, 5000);
		tolayer3(0, universalAPkt);
		SendStatus = NOT_READY;
	}
}

/*
 * Just like A_output, but residing on the B side.  USED only when the 
 * implementation is bi-directional.
 */
void B_output(struct msg message) {
	// Do Not Require
}

/* 
 * A_input(packet), where packet is a structure of type pkt. This routine 
 * will be called whenever a packet sent from the B-side (i.e., as a result 
 * of a tolayer3() being done by a B-side procedure) arrives at the A-side. 
 * packet is the (possibly corrupted) packet sent from the B-side.
 */
void A_input(struct pkt packet) {
	char str[20];
	memset(str, '\0', sizeof(str));

	if ((!strcmp(packet.payload, str))
			&& (packet.checksum == checkSum(packet.payload))
			&& (packet.acknum == 1 && packet.seqnum == universalAPkt.seqnum)) {
		SendStatus = READY;
		char *buffer = dequeue();
		if (buffer != NULL) {
			struct msg newMessage;
			strncpy(newMessage.data, buffer, 20);
			A_output(newMessage);
		}
	}
}

/*
 * A_timerinterrupt()  This routine will be called when A's timer expires 
 * (thus generating a timer interrupt). You'll probably want to use this 
 * routine to control the retransmission of packets. See starttimer() 
 * and stoptimer() in the writeup for how the timer is started and stopped.
 */
void A_timerinterrupt() {
	stopTimer(0);
	SendStatus = READY;
	struct msg newMessage;
	strncpy(newMessage.data, universalAPkt.payload, 20);
	universalAPkt.seqnum = flipBit(universalAPkt.seqnum);
	A_output(newMessage);
}

/* The following routine will be called once (only) before any other    */
/* entity A routines are called. You can use it to do any initialization */
void A_init() {
	universalAPkt.seqnum = 0;
	universalAPkt.acknum = 1;
	SendStatus = READY;
}

/* 
 * Note that with simplex transfer from A-to-B, there is no routine  B_output() 
 */

/*
 * B_input(packet),where packet is a structure of type pkt. This routine 
 * will be called whenever a packet sent from the A-side (i.e., as a result 
 * of a tolayer3() being done by a A-side procedure) arrives at the B-side. 
 * packet is the (possibly corrupted) packet sent from the A-side.
 */
void B_input(struct pkt packet) {
	if (PacketStatus(packet) == CORRUPT) {
		universalBPkt.seqnum = previousSeqNum;
		universalBPkt.checksum = checkSum(universalBPkt.payload);
		tolayer3(1, universalBPkt);
		return;
	} else if (packet.seqnum == previousSeqNum) {
		universalBPkt.seqnum = previousSeqNum;
		universalBPkt.checksum = checkSum(universalBPkt.payload);
		tolayer3(1, universalBPkt);
	} else {
		previousSeqNum = packet.seqnum;
		universalBPkt.seqnum = packet.seqnum;
		universalBPkt.checksum = checkSum(universalBPkt.payload);
		tolayer3(1, universalBPkt);

		struct msg newMessage;
		strncpy(newMessage.data, packet.payload, 20);
		tolayer5(1, newMessage);

	}

}

/*
 * B_timerinterrupt()  This routine will be called when B's timer expires 
 * (thus generating a timer interrupt). You'll probably want to use this 
 * routine to control the retransmission of packets. See starttimer() 
 * and stoptimer() in the writeup for how the timer is started and stopped.
 */
void B_timerinterrupt() {
	// Do Not Require
}

/* 
 * The following routine will be called once (only) before any other   
 * entity B routines are called. You can use it to do any initialization 
 */
void B_init() {
	previousSeqNum = 0;
	universalBPkt.acknum = 1;
	memset(universalBPkt.payload, '\0', sizeof(universalBPkt.payload));
}


#include "../include/simulator.h"
#include <vector>
#include <iostream>
#include <stdlib.h>
#include <string.h>
using namespace std;
/* ******************************************************************
 ALTERNATING BIT AND GO-BACK-N NETWORK EMULATOR: VERSION 1.1  J.F.Kurose

   This code should be used for PA2, unidirectional data transfer
   protocols (from A to B). Network properties:
   - one way network delay averages five time units (longer if there
     are other messages in the channel for GBN), but can be larger
   - packets can be corrupted (either the header or the data portion)
     or lost, according to user-defined probabilities
   - packets will be delivered in the order in which they were sent
     (although some can be lost).
**********************************************************************/


/********* STUDENTS WRITE THE NEXT SEVEN ROUTINES *********/
int base;
int nextseqnum;
float timeout;
int windowSize;
struct pkt* buffer[1500];
int B_SeqNo_expected;

void makePacket_putInBuffer();
int makeChecksum();

int makeChecksum(struct pkt packet){
    char msg[20];
    strcpy(msg,packet.payload);
    int checksum = 0;
    for(int i = 0; i<20 && msg[i] != '\0'; i++){
        checksum += msg[i];
    }
    checksum += packet.seqnum;
    checksum += packet.acknum;
    return checksum;
}

void makePacket_putInBuffer(int seq, struct msg message)
{
    struct pkt *newPacket = (struct pkt *)malloc(sizeof(struct pkt));
    newPacket->acknum = 0;
    newPacket->seqnum = nextseqnum;
    strcpy(newPacket->payload,message.data);
    newPacket->checksum = makeChecksum((*newPacket));
    buffer[nextseqnum] = newPacket;
}

/* called from layer 5, passed the data to be sent to other side */
void A_output(struct msg message)
{
    makePacket_putInBuffer(nextseqnum,message);
    if(base == nextseqnum)
    {
      starttimer(0,timeout);
    }
    if(nextseqnum < base + windowSize)
    {
      tolayer3(0,*(buffer[nextseqnum]));
    }
    nextseqnum++;
}

/* called from layer 3, when a packet arrives for layer 4 */
void A_input(struct pkt packet)
{
     if( makeChecksum(packet) == packet.checksum && packet.acknum >= base )
     {
        int oldbase = base;
        base = packet.acknum + 1;
        stoptimer(0);

        for(int i = oldbase + windowSize - 1; i < nextseqnum && i < base + windowSize; i++)
        {
             tolayer3(0,*(buffer[i]));
        }
        starttimer(0,timeout);
     }
}

/* called when A's timer goes off */
void A_timerinterrupt()
{
    int freeWindow = base + windowSize;
    for(int i = base; i < freeWindow && i < nextseqnum; i++)
    {
         tolayer3(0,*(buffer[i]));
    }
    starttimer(0,timeout);
}

/* the following routine will be called once (only) before any other */
/* entity A routines are called. You can use it to do any initialization */
void A_init()
{
   windowSize = getwinsize();
   base = 0;
   nextseqnum = 0;
   timeout = 20.0;
}

/* called from layer 3, when a packet arrives for layer 4 at B*/
void B_input(struct pkt packet)
{
    if(makeChecksum(packet) == packet.checksum && packet.seqnum == B_SeqNo_expected){
      tolayer5(1, packet.payload);
      struct pkt *ackPacket = (struct pkt *)malloc(sizeof(struct pkt *));
      ackPacket->acknum = B_SeqNo_expected;
      ackPacket->seqnum = 0;
      ackPacket->checksum = makeChecksum((*ackPacket));
      tolayer3(1, *ackPacket);
      B_SeqNo_expected++;
    }
    else if(makeChecksum(packet) == packet.checksum && packet.seqnum < B_SeqNo_expected)
    {
      struct pkt *ackPacket = (struct pkt *)malloc(sizeof(struct pkt *));
      ackPacket->acknum = B_SeqNo_expected - 1;
      ackPacket->seqnum = 0;
      ackPacket->checksum = makeChecksum((*ackPacket));
      tolayer3(1, *ackPacket);
    }
}

/* the following rouytine will be called once (only) before any other */
/* entity B routines are called. You can use it to do any initialization */
void B_init()
{
    B_SeqNo_expected=0;
}

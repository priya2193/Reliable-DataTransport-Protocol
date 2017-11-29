#include "../include/simulator.h"

#include <vector>
#include <queue>
#include <iostream>
#include <stdlib.h>
#include <string.h>

using namespace std;

int makeChecksum();
char* fetch_data(struct msg);

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

/********* STUDENTS WRITE THE NEXT SIX ROUTINES *********/

struct pkt sendPkt;
static int A_SeqNo_expected ;
static int B_SeqNo_expected ;
static vector<struct msg> buffer ;
int flag ;
float timeout ;


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

char* fetch_data(struct msg Mess){
  return Mess.data;
}

/* called from layer 5, passed the data to be sent to other side */
void A_output(struct msg message)
{
  if(strcmp(message.data,"Null") != 0){
    buffer.push_back(message);
  }
  if(flag == 1){
      flag = 0;
      std::vector<msg>::iterator it = buffer.begin();
      char *me = fetch_data((*it));
      struct pkt *newPacket = (struct pkt *)malloc(sizeof(struct pkt));
      newPacket->seqnum = A_SeqNo_expected;
      newPacket->acknum = 0;
      strcpy(newPacket->payload,me);
      newPacket->checksum = makeChecksum((*newPacket));
      starttimer(0,timeout);
      sendPkt = *newPacket;
      tolayer3(0, *newPacket);
  }
}

/* called from layer 3, when a packet arrives for layer 4 */
void A_input(struct pkt packet)
{
  if(makeChecksum(packet) == packet.checksum && packet.acknum == A_SeqNo_expected){
      flag = 1;
      stoptimer(0);
      A_SeqNo_expected = !A_SeqNo_expected;
      struct msg Message;
      strcpy(Message.data, "Null");
      buffer.erase (buffer.begin());
      if(buffer.size() != 0){
        A_output(Message);
      }
  }
}

/* called when A's timer goes off */
void A_timerinterrupt()
{
  if(flag == 0){
      starttimer(0,timeout);
      tolayer3(0, sendPkt);
  }
}

/* called from layer 3, when a packet arrives for layer 4 at B*/
void B_input(struct pkt packet)
{
  if(B_SeqNo_expected == packet.seqnum && makeChecksum(packet) == packet.checksum){
      tolayer5(1,packet.payload);
      struct pkt *ackPacket = (struct pkt *)malloc(sizeof(struct pkt *));
      ackPacket->acknum = packet.seqnum;
      ackPacket->seqnum = 0;
      ackPacket->checksum = makeChecksum((*ackPacket));
      tolayer3(1, (*ackPacket));
      B_SeqNo_expected = !B_SeqNo_expected;
  }
  else if(B_SeqNo_expected != packet.seqnum && makeChecksum(packet) == packet.checksum){
      struct pkt (*ackPacket) = (struct pkt *) malloc(sizeof(struct pkt *));
      ackPacket->acknum = packet.seqnum;
      ackPacket->seqnum = 0;
      ackPacket->checksum = makeChecksum((*ackPacket));
      tolayer3(1, (*ackPacket));
  }
}

/* the following routine will be called once (only) before any other */
/* entity A routines are called. You can use it to do any initialization */
void A_init()
{
  timeout = 20;
  flag = 1;
  A_SeqNo_expected = 0;
}

/* the following routine will be called once (only) before any other */
/* entity B routines are called. You can use it to do any initialization */
void B_init()
{
  B_SeqNo_expected = 0;
}

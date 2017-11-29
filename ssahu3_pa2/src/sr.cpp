#include "../include/simulator.h"
#include <vector>
#include <map>
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


int A_windowSize ;
int A_base ;
int nextseqnum ;
float timeout ;
int B_base ;
int B_windowSize ;
int B_SeqNo_expected ;
int RecvSize = 1100;

struct A_pktData{
  struct pkt *packet;
  int startTime;
  bool acknowledged;
  A_pktData(struct pkt *pack)
  {
    packet = pack;
    int startTime = 0;
    bool acknowledged = false;
  }
};

static vector<struct A_pktData *> A_buffer;

struct B_pktData
{
  struct pkt packet;
  bool isRecv;
};

struct B_pktData *B_buffer;

int makeChecksum();
void setTime();
void makePacket_putInBuffer();
void setAck();

void setTime(int seq) {
  (A_buffer[seq])->startTime = get_sim_time();
}

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
  A_buffer.push_back( new (struct A_pktData)(newPacket));
}

/* called from layer 5, passed the data to be sent to other side */
void A_output(struct msg message)
{
  std::cout << "IN A_output " << '\n';
  makePacket_putInBuffer(nextseqnum,message);
  if(A_base == nextseqnum)
  {
    starttimer(0,timeout);
  }
  if(nextseqnum < A_base + A_windowSize)
  {
    setTime(nextseqnum);
    tolayer3(0,*(A_buffer[nextseqnum]->packet));
  }
  nextseqnum++;
}

/* called from layer 3, when a packet arrives for layer 4 */
void A_input(struct pkt packet)
{
  if( makeChecksum(packet) == packet.checksum && packet.acknum >= A_base && packet.acknum < (A_base + A_windowSize))
  {
    (A_buffer[packet.acknum])->acknowledged = true;
    if(A_base == packet.acknum)
    {
      stoptimer(0);
      A_base = A_base + 1;
      for(A_base; A_base < nextseqnum; A_base++)
      {
        if(A_base+A_windowSize < nextseqnum && (A_buffer[A_base + A_windowSize])->acknowledged == false){
          tolayer3(0,*((A_buffer[A_base + A_windowSize])->packet));
          setTime(A_base+A_windowSize);
        }
        if((A_buffer[A_base])->acknowledged == false){
          int remainingTime = timeout - (get_sim_time() - ((A_buffer[A_base])->startTime ));
          starttimer(0,remainingTime);
          break;
        }
      }
    }
  }
}

/* called when A's timer goes off */
void A_timerinterrupt()
{
  float minTime = (A_buffer[A_base])->startTime ;
  int lowTimerPkt = A_base;
  for(int i = A_base ; i<A_base + A_windowSize && i < nextseqnum; i++)
  {
    if((A_buffer[i])->acknowledged == false)//ELAY
    {
      if((A_buffer[i])->startTime < minTime )
      {
        minTime = (A_buffer[i])->startTime;
        lowTimerPkt = i;
      }
    }
  }

  tolayer3(0,*(A_buffer[lowTimerPkt]->packet));
  setTime(lowTimerPkt);

  float secMinTime = (A_buffer[A_base])->startTime ;
  int secLowTimerPkt = A_base;

  for(int j = A_base ; j < A_base + A_windowSize && j < nextseqnum; j++)
  {
    if((A_buffer[j])->acknowledged == false)//ELAY
    {
      if((A_buffer[j])->startTime < secMinTime )
      {
        secMinTime = (A_buffer[j])->startTime;
        secLowTimerPkt = j;
      }
    }
  }
  int remainingTime = timeout - (get_sim_time() - ((A_buffer[secLowTimerPkt])->startTime ));
  if (remainingTime > 0)
  {
    starttimer(0,remainingTime);
  } else {
    starttimer(0,timeout);
  }
}


/* called from layer 3, when a packet arrives for layer 4 at B*/
void B_input(struct pkt packet)
{
  std::cout << "Inside B_input"<< '\n';
  if(makeChecksum(packet) == packet.checksum && packet.seqnum >= B_base && packet.seqnum < (B_base + B_windowSize) ){

    int B_place = packet.seqnum;
    B_buffer[B_place].packet = packet;
    B_buffer[B_place].isRecv = true;

    struct pkt *ackPacket = (struct pkt *)malloc(sizeof(struct pkt *));
    ackPacket->acknum = packet.seqnum;
    ackPacket->seqnum = 0;
    ackPacket->checksum = makeChecksum((*ackPacket));
    tolayer3(1, *ackPacket);

    for(int i=B_base; i < (B_base + B_windowSize) ; i++){
      if (B_buffer[i].isRecv == false){
        break;
      }else{
        B_base++;
        tolayer5(1,B_buffer[i].packet.payload);
      }
    }
  }
  else if(makeChecksum(packet) == packet.checksum && (packet.seqnum < B_base))
  {
    struct pkt *ackPacket = (struct pkt *)malloc(sizeof(struct pkt *));
    ackPacket->acknum = packet.seqnum;
    ackPacket->seqnum = 0;
    ackPacket->checksum = makeChecksum((*ackPacket));
    tolayer3(1, *ackPacket);
  }
}

/* the following routine will be called once (only) before any other */
/* entity A routines are called. You can use it to do any initialization */
void A_init()
{
  A_windowSize = getwinsize();
  A_base = 0;
  nextseqnum = 0;
  timeout = 10.0;
}

/* the following rouytine will be called once (only) before any other */
/* entity B routines are called. You can use it to do any initialization */
void B_init()
{
  B_base =0;
  B_windowSize = getwinsize();
  B_SeqNo_expected=0;
  B_buffer = (struct B_pktData*)malloc(RecvSize*sizeof(struct B_pktData));
}

/*******************************************/
/* @file packet.h                          */
/*                                         */
/* @brief Header files for packet.c        */
/*                                         */
/* @author Malek Anabtawi, Fadhil Abubaker */
/*******************************************/

#ifndef PACKET_H
#define PACKET_H

typedef struct packet {
  char  type;
  short hlen;
  short plen;
  int   seqno;
  int   ackno;
} packet;

#endif

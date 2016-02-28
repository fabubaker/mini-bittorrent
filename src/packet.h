/********************************************************/
/* @file packet.h                                       */
/*                                                      */
/* @brief Header for packet.c                           */
/*                                                      */
/* @author Malek Anabtawi, Fadhil Abubaker              */
/********************************************************/

#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include "chunk.h"
#include "ds.h"

#define MAX_NUM_HASH    74 // Maximum possible number of chunks in a packet
#define PACKET_LENGTH   1500 // Maximum packet length

#define HEADER          16 // Number of bytes in the header
#define CHUNK           20 // Number of bytes in the chunk

#define MAGIC_NUMBER    15441 // The magic number
#define VERSION_NUMBER  1 // The version number

#define WHOHAS_TYPE     0
#define IHAVE_TYPE      1
#define GET_TYPE        2
#define DATA_TYPE       3
#define ACK_TYPE        4
#define DENIED_TYPE     5


struct byte_buf {
  uint8_t buf[PACKET_LENGTH];
  int pos;
};

typedef struct byte_buf byte_buf;

void mmemcat(byte_buf *tempRequest, uint8_t *binaryNumber, int size);
void dec2hex2binary(int decimalNumber, int bytesNeeded, uint8_t* binaryNumber);

void gen_WHOHAS(ll *list);
void gen_IHAVE();

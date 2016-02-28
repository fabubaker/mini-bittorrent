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

#define MAX_NUM_HASH    74 //Maximum possible number of chunks in a packet
#define WHOHAS_HEADER   16 //Number of bytes in the header of WHOHAS
#define WHOHAS_CHUNK    20 //Number of bytes in a WHOHAS hash
#define MAGIC_NUMBER    15441 //The magic number
#define VERSION_NUMBER  1 //The version number
#define WHOHAS_TYPE     0 //The packet type for WHOHAS
#define PACKET_LENGTH   1500 //Maximum packet length

struct temp_info{
  uint8_t buf[PACKET_LENGTH];
  int pos;
};

typedef struct temp_info temp_info;

void mmemcat(temp_info *tempRequest, uint8_t *binaryNumber, int size);
void dec2hex2binary(int decimalNumber, int bytesNeeded, uint8_t* binaryNumber);
void gen_WHOHAS(ll *myHead);

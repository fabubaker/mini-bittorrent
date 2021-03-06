/********************************************************/
/* @file packet.h                                       */
/*                                                      */
/* @brief Header for packet.c                           */
/*                                                      */
/* @author Malek Anabtawi, Fadhil Abubaker              */
/********************************************************/

#ifndef PACKET_H
#define PACKET_H

#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include "chunk.h"
#include "ds.h"
#include "uthash.h"
#include "peer.h"

#define DATA_LENGTH	    1024  // Size of the data in a DATA packet
#define CHUNK_SIZE		  524288// Size of a data chunk in bytes
#define MAX_NUM_HASH    74 	  // Maximum possible number of chunks in a packet
#define PACKET_LENGTH   1500  // Maximum packet length

#define HEADER          16    // Number of bytes in the header
#define CHUNK           20    // Number of bytes in the chunk

#define MAGIC_NUMBER    15441 // The magic number
#define VERSION_NUMBER  1     // The version number

#define WHOHAS_TYPE     0
#define IHAVE_TYPE      1
#define GET_TYPE        2
#define DATA_TYPE       3
#define ACK_TYPE        4
#define DENIED_TYPE     5

/* Structs */

struct byte_buf {
  uint8_t *buf;
  int pos;
  size_t bufsize;
};

struct packet_info {
    uint8_t magicNumber[2];
    uint8_t versionNumber[1];
    uint8_t packetType[1];
    uint8_t headerLength[2];
    uint8_t totalPacketLength[2];
    uint8_t sequenceNumber[4];
    uint8_t ackNumber[4];
    uint8_t numberHashes[1];
    uint8_t padding[3]; // never be used but needed
    // Not the padding we need, but the padding we deserve...
    // What the hell m8
    uint8_t body[PACKET_LENGTH];
};

typedef struct byte_buf byte_buf;
typedef struct packet_info packet_info;
typedef struct chunk_table chunk_table;
typedef struct peer        peer;

void mmemmove(uint8_t *binaryNumber, byte_buf *tempRequest, int size);
void mmemcat(byte_buf *tempRequest, uint8_t *binaryNumber, int size);
void mmemclear(byte_buf *buf);

void dec2hex2binary(int decimalNumber, int bytesNeeded, uint8_t* binaryNumber);
int get_seqno(uint8_t* packet);

ll* gen_WHOIGET(ll *list, int packetCode);
ll* gen_DATA(uint8_t *chunkHash);
void parse_packet(uint8_t *packet, packet_info* myPack);
void parse_data(packet_info* packetinfo, peer* p);
void save2file(chunk_table* chunk);

void print_packet(uint8_t* packet, int i);

struct byte_buf*  create_bytebuf(size_t bufsize);
void              delete_bytebuf(struct byte_buf* buf);

#endif

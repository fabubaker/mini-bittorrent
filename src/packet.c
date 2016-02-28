/********************************************************/
/* @file packet.c                                       */
/*                                                      */
/* @brief Library to handle packet creation and parsing */
/*                                                      */
/* @author Malek Anabtawi, Fadhil Abubaker              */
/********************************************************/

#include "packet.h"

void mmemcat(byte_buf *tempRequest, uint8_t *binaryNumber, int size){
    memmove(tempRequest->buf + tempRequest->pos, binaryNumber, size);
    tempRequest->pos += size;
}

/**
 * converts the binary char string str to ascii format. the length of
 * ascii should be 2 times that of str
 */
void binary2hex(uint8_t *buf, int len, char *hex) {
    int i=0;
    for(i=0;i<len;i++) {
        sprintf(hex+(i*2), "%.2x", buf[i]);
    }
    hex[len*2] = 0;
}

/**
 *Ascii to hex conversion routine
 */
static uint8_t _hex2binary(char hex)
{
  hex = toupper(hex);
  uint8_t c = ((hex <= '9') ? (hex - '0') : (hex - ('A' - 0x0A)));
  return c;
}

/**
 * converts the ascii character string in "ascii" to binary string in "buf"
 * the length of buf should be atleast len / 2
 */
void hex2binary(char *hex, int len, uint8_t*buf) {
    int i = 0;
    for(i=0;i<len;i+=2) {
        buf[i/2] =  _hex2binary(hex[i]) << 4
      | _hex2binary(hex[i+1]);
    }
}

void dec2hex2binary(int decimalNumber, int bytesNeeded, uint8_t* binaryNumber){

    int quotient;
    int i=bytesNeeded-1, temp;
    char hexadecimalNumber[bytesNeeded];
    memset(hexadecimalNumber, 0, bytesNeeded);
    quotient = decimalNumber;

    while(quotient!=0){
        temp = quotient % 16;

        if(temp < 10){
            temp = temp + 48;
        } else {
            temp = temp + 55;
        }

        hexadecimalNumber[i] = temp;
        quotient = quotient / 16;
        i--;
    }
    hex2binary(hexadecimalNumber, bytesNeeded, binaryNumber);
}

/* Generates a WHOHAS request (more than one if necessary). Each packet
 * has its own header.
 * Arguments:
 *      1. list: Pre-processed linked list. The head contains the number of
 *      chunks and a first node, which corresponds to the first chunk.
 */
void gen_WHOHAS(ll *list){

    int numHashes = list->count;
    int numPacket = (numHashes / MAX_NUM_HASH) + 1;
    int packCounter = 0;
    int tempNumHashes;
    uint8_t Requests[numPacket][PACKET_LENGTH];
    node *temp = list->first;
    byte_buf tempRequest;

    uint8_t magicNumber[2];
    uint8_t versionNumber[1];
    uint8_t packetType[1] = {WHOHAS_TYPE};
    uint8_t headerLength[2];
    uint8_t totalPacketLength[2];
    uint8_t sequenceNumber[4] = {0,0,0,0};
    uint8_t ackNumber[4] = {0,0,0,0};
    uint8_t padding[3] = {0,0,0};
    uint8_t numberHashes[1];

    while(packCounter < numPacket){
        if(numPacket == packCounter + 1){
            tempNumHashes = numHashes;
        } else {
            tempNumHashes = MAX_NUM_HASH;
        }

        bzero(tempRequest.buf, PACKET_LENGTH);
        tempRequest.pos = 0;

        dec2hex2binary(MAGIC_NUMBER, 4, magicNumber);
        dec2hex2binary(VERSION_NUMBER, 2, versionNumber);
        dec2hex2binary(HEADER, 4, headerLength);
        dec2hex2binary(HEADER + CHUNK * tempNumHashes + 4, 4,
                                                        totalPacketLength);
        dec2hex2binary(tempNumHashes, 2, numberHashes);
        mmemcat(&tempRequest, magicNumber,       2);
        mmemcat(&tempRequest, versionNumber,     1);
        mmemcat(&tempRequest, packetType,        1);
        mmemcat(&tempRequest, headerLength,      2);
        mmemcat(&tempRequest, totalPacketLength, 2);
        mmemcat(&tempRequest, sequenceNumber,    4);
        mmemcat(&tempRequest, ackNumber,         4);
        mmemcat(&tempRequest, numberHashes,      1);
        mmemcat(&tempRequest, padding,           3);

        while(temp != NULL){
            mmemcat(&tempRequest, temp->data, CHUNK);
            temp = temp->next;
        }

        memcpy(Requests[packCounter], tempRequest.buf, PACKET_LENGTH);
        packCounter++;
        numHashes -= MAX_NUM_HASH;
    }
}

#ifdef TESTING
int main(){
  uint8_t buf1[200] = {2,3,4,1,1,1,0};
  uint8_t buf2[200] = {1,3,2,1,1,1,0};
  uint8_t buf3[200] = {6,5,13,2,1,8,0};
  //  node* cur = NULL;

  ll* test1 = create_ll();
  add_node(test1, buf1, 200);
  add_node(test1, buf2, 200);
  add_node(test1, buf3, 200);
  add_node(test1, buf3, 200);
  add_node(test1, buf3, 200);
  add_node(test1, buf3, 200);
  add_node(test1, buf3, 200);
  add_node(test1, buf3, 200);

  gen_WHOHAS(test1);

}
#endif

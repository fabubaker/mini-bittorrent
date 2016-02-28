/********************************************************/
/* @file packet.c                                       */
/*                                                      */
/* @brief Library to handle packet creation and parsing */
/*                                                      */
/* @author Malek Anabtawi, Fadhil Abubaker              */
/********************************************************/

#include <stdio.h>
#include "chunk.h"

#define MAX_NUM_HASH	74 //Maximum possible number of chunks in a packet
#define WHOHAS_HEADER	20 //Number of bytes in the header of WHOHAS
#define WHOHAS_CHUNK	20 //Number of bytes in a WHOHAS hash


//Stolen as shit
void dec2hex2binary(int decimalNumber, int bytesNeeded, uint8_t binaryNumber){

    int remainder,quotient;
    int i,temp;
    char hexadecimalNumber[bytesNeeded];
    quotient = decimalNumber;

    while(quotient!=0){
    	temp = quotient % 16;

        if(temp < 10){
        	temp = temp + 48;
        } else {
        	temp = temp + 55;
        }

        hexadecimalNumber[i]= temp;
        quotient = quotient / 16;
        i++;
    }

    hex2binary(hexadecimalNumber, bytesNeeded, binaryNumber);
}

/* Generates a WHOHAS request (more than one if necessary). Each packet
 * has its own header.
 * Arguments:
 * 		1. myHead: Pre-processed linked list. The head contains the number of
 * 		chunks and a first node, which corresponds to the first chunk.
 *
char** gen_WHOHAS(ll *myHead){
	int numHashes = myHead->count;
	int numPacket = (numHashes / MAX_NUM_HASH) + 1;
	uint8_t *magicNumber[2];
	uint8_t versionNumber[1];
	uint8_t packetType[1];
	uint8_t headerLength[2];
	uint8_t totalPacketLength[2];
	uint8_t sequenceNumber[4];
	uint8_t ackNumber[4];
	uint8_t numberHashes[1];
	uint8_t padding[3];
}
*/

int main(){
	uint8_t *magicNumber[2];
	dec2hex2binary("15441", 4, magicNumber);
	printf("My number: %02x and %02x", magicNumber[0], magicNumber[1]);
	return 0;
}
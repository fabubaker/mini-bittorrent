/********************************************************/
/* @file packet.c                                       */
/*                                                      */
/* @brief Library to handle packet creation and parsing */
/*                                                      */
/* @author Malek Anabtawi, Fadhil Abubaker              */
/********************************************************/

/*
 * TODO: I've changed add_node to take in an 'int type'. We can use
 * it to differentiate between sliding window packets and non sliding window
 * packs. Change your add_node calls.
 *
 * TODO: I've made an append function for linked lists. Use it to append
 * p->tosend and gen_* linked lists.
 *
 */


#include "packet.h"

/* Globals */

extern  peer*        peer_list;      // Provided in argv

extern  chunk_table* get_chunks;     // Provided in STDIN
extern  chunk_table* has_chunks;     // Provided in argv
extern  chunk_table* master_chunks;  // Provided in argv

extern size_t       max_conn;          // Provided in argv

extern char*        master_data_file;  // Provided in master_chunks file
extern char*        output_file;       // Provided in STDIN

/* Definitions */

struct byte_buf* create_bytebuf(size_t bufsize)
{
  struct byte_buf *b;
  b = malloc(sizeof(struct byte_buf));
  if (!b) return NULL;

  b->buf = malloc(bufsize + 1);
  if (!b->buf) {
    free(b);
    return NULL;
  }

  b->pos = 0;
  bzero(b->buf, bufsize+1);

  b->bufsize = bufsize;

  return b;
}

void mmemmove(uint8_t *binaryNumber, byte_buf *tempRequest, int size){
  memmove(binaryNumber, tempRequest->buf + tempRequest->pos, size);
  tempRequest->pos += size;
}

void mmemcat(byte_buf *tempRequest, uint8_t *binaryNumber, int size){
  memmove(tempRequest->buf + tempRequest->pos, binaryNumber, size);
  tempRequest->pos += size;
}

void mmemclear(byte_buf *b)
{
  b->pos = 0;
  bzero(b->buf, b->bufsize);
}

int binary2int(uint8_t *buf, int len){
  char temp[2*len];
  bzero(temp, 2*len);
  binary2hex(buf, len, temp);
  int ret = (int)strtol(temp, NULL, 16);
  return ret;
}

/* First converts decimalNumber from decimal to hex. Next, it passes the
 * result to the given hex2binary function.
 * Arguments:
 *      1. decimalNumber: Number to be converted.
 *      2. bytesNeeded: The maximum possible number of hex bytes needed to
 *         represent the number.
 *      3. binaryNumber: buffer where the binary number is stored
 */
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

  i = 0;

  while(i < bytesNeeded){
    if(hexadecimalNumber[i] != '\0'){
      break;
    }
    hexadecimalNumber[i] = '0';
    i++;
  }

  hex2binary(hexadecimalNumber, bytesNeeded, binaryNumber);
}


/* Pareses a given, complete packet.
 * Arguments:
 *      1. packet: The actual packet to be parsed.
 *      2. myPack: A struct that will contain information about the packet.
 */
void parse_packet(uint8_t *packet, packet_info* myPack){

  byte_buf *tempRequest = create_bytebuf(PACKET_LENGTH);
  mmemclear(tempRequest);

  memcpy(tempRequest->buf, packet, PACKET_LENGTH);

  mmemmove(myPack->magicNumber, tempRequest,       2);
  mmemmove(myPack->versionNumber, tempRequest,     1);

  /*
    if(magicNumber != 15441 && versionNumber != 1){
    //Drop the packet
    }
  */

  mmemmove(myPack->packetType,         tempRequest, 1);
  mmemmove(myPack->headerLength,       tempRequest, 2);
  mmemmove(myPack->totalPacketLength,  tempRequest, 2);
  mmemmove(myPack->sequenceNumber,     tempRequest, 4);
  mmemmove(myPack->ackNumber,          tempRequest, 4);

  int packetType = binary2int(myPack->packetType, 1);

  if(packetType == 0 || packetType == 1 || packetType == 2){

    mmemmove(myPack->numberHashes, tempRequest,    1);
    mmemmove(myPack->padding, tempRequest,         3);
  }

  int headerLength = binary2int(myPack->headerLength, 2);
  int totalPacketLength = binary2int(myPack->totalPacketLength, 2);

  mmemmove(myPack->body, tempRequest, totalPacketLength - headerLength);
}

//Test later
ll* gen_ACK(int ackNum, int copies){

  byte_buf *tempRequest = create_bytebuf(PACKET_LENGTH);
  mmemclear(tempRequest);
  ll* myLL = create_ll();

  uint8_t magicNumber[2];
  uint8_t versionNumber[1] = {VERSION_NUMBER};
  uint8_t packetType[1] = {DATA_TYPE};
  uint8_t headerLength[2];
  uint8_t totalPacketLength[2];
  uint8_t sequenceNumber[4] = {0,0,0,0};
  uint8_t ackNumber[4];


  bzero(magicNumber, 2);
  bzero(headerLength, 2);
  bzero(totalPacketLength, 2);
  bzero(ackNumber, 4);

  dec2hex2binary(MAGIC_NUMBER, 4, magicNumber);
  dec2hex2binary(HEADER, 4, headerLength);
  dec2hex2binary(HEADER + DATA_LENGTH, 4, totalPacketLength);
  dec2hex2binary(ackNum, 8, ackNumber);

  mmemcat(tempRequest, magicNumber,       2);
  mmemcat(tempRequest, versionNumber,     1);
  mmemcat(tempRequest, packetType,        1);
  mmemcat(tempRequest, headerLength,      2);
  mmemcat(tempRequest, totalPacketLength, 2);
  mmemcat(tempRequest, sequenceNumber,    4);
  mmemcat(tempRequest, ackNumber,         4);

  while(copies){
    add_node(myLL, tempRequest->buf, HEADER + DATA_LENGTH);
    copies--;
  }
  return myLL;
}

//Test later
/* Generates a complete set of DATA packets for an entire chunk.
 * Arguments:
 *      1. Requests: Array of uint8_ts. Must have space for at least 512 spots.
 *      2. chunkHash: The required chunk to send.
 */
ll* gen_DATA(uint8_t *chunkHash) {

  byte_buf *tempRequest = create_bytebuf(PACKET_LENGTH);
  mmemclear(tempRequest);

  int id;
  uint8_t buf[CHUNK_SIZE];
  uint8_t tempBuf[DATA_LENGTH];
  bzero(buf, CHUNK_SIZE);
  chunk_table *lookup;
  ll *myLL = create_ll();

  HASH_FIND(hh, has_chunks, chunkHash, 20, lookup);

  if(lookup == NULL){
    //Error!
  } else {
    id = (int)lookup->id;
  }

  FILE *fp = fopen(master_data_file, "r");

  if(!(fseek(fp, id * CHUNK_SIZE, 0))){
    //Error!
  }

  if(fread((char *)buf, 1, CHUNK_SIZE, fp) == 0){ //BROKEN?
    //Short count!
  }

  int numPacket = 512;
  int packCounter = 0;
  int bufPos = 0;
  int seqNumber = 1;

  uint8_t magicNumber[2];
  uint8_t versionNumber[1] = {VERSION_NUMBER};
  uint8_t packetType[1] = {DATA_TYPE};
  uint8_t headerLength[2];
  uint8_t totalPacketLength[2];
  uint8_t sequenceNumber[4];
  uint8_t ackNumber[4] = {0,0,0,0};

  while(packCounter < numPacket){

    bzero(magicNumber, 2);
    bzero(headerLength, 2);
    bzero(totalPacketLength, 2);
    bzero(sequenceNumber, 4);
    bzero(tempBuf, DATA_LENGTH);

    memcpy(tempBuf, buf + bufPos, DATA_LENGTH);
    dec2hex2binary(MAGIC_NUMBER, 4, magicNumber);
    dec2hex2binary(HEADER, 4, headerLength);
    dec2hex2binary(HEADER + DATA_LENGTH, 4, totalPacketLength);
    dec2hex2binary(seqNumber, 8, sequenceNumber);
    mmemclear(tempRequest);

    mmemcat(tempRequest, magicNumber,       2);
    mmemcat(tempRequest, versionNumber,     1);
    mmemcat(tempRequest, packetType,        1);
    mmemcat(tempRequest, headerLength,      2);
    mmemcat(tempRequest, totalPacketLength, 2);
    mmemcat(tempRequest, sequenceNumber,    4);
    mmemcat(tempRequest, ackNumber,         4);
    mmemcat(tempRequest, tempBuf, DATA_LENGTH);

    add_node(myLL, tempRequest->buf, HEADER + DATA_LENGTH);
    packCounter++;
    bufPos += DATA_LENGTH;
  }
  return myLL;
}

/* Generates a WHOHAS/IHAVE/GET request (more than one if necessary). Each
 * packet has its own header. Which request is generated depends on the
 * packetCode argument.
 * Arguments:
 *      1. list: Pre-processed linked list. The head contains the number of
 *      chunks and a first node, which corresponds to the first chunk.
 *      2. packetCode: 0 for WHOHAS, 1 for IHAVE, 2 for GET (the real GET).
 */
ll* gen_WHOIGET(ll *list, int packetCode){

  int numHashes = list->count;
  int numPacket = (numHashes / MAX_NUM_HASH) + 1;
  int packCounter = 0;
  int tempNumHashes;

  ll *myLL = create_ll();

  node *temp = list->first;
  byte_buf *tempRequest = create_bytebuf(PACKET_LENGTH);
  mmemclear(tempRequest);

  /*
    if(packetCode == 2){
    ASSERT(numHashes == 1);
    }
  */

  uint8_t magicNumber[2];
  uint8_t versionNumber[1] = {VERSION_NUMBER};
  uint8_t packetType[1] = {packetCode};
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

    bzero(magicNumber, 2);
    bzero(headerLength, 2);
    bzero(totalPacketLength, 2);
    bzero(numberHashes, 1);

    mmemclear(tempRequest);

    dec2hex2binary(MAGIC_NUMBER, 4, magicNumber);
    dec2hex2binary(HEADER, 4, headerLength);
    dec2hex2binary(HEADER + CHUNK * tempNumHashes + 4, 4,
                   totalPacketLength);
    dec2hex2binary(tempNumHashes, 2, numberHashes);

    mmemcat(tempRequest, magicNumber,       2);
    mmemcat(tempRequest, versionNumber,     1);
    mmemcat(tempRequest, packetType,        1);
    mmemcat(tempRequest, headerLength,      2);
    mmemcat(tempRequest, totalPacketLength, 2);
    mmemcat(tempRequest, sequenceNumber,    4);
    mmemcat(tempRequest, ackNumber,         4);
    mmemcat(tempRequest, numberHashes,      1);
    mmemcat(tempRequest, padding,           3);

    while(temp != NULL){
      mmemcat(tempRequest, temp->data, CHUNK); // No
      temp = temp->next;
    }

    add_node(myLL, tempRequest->buf, HEADER + CHUNK * tempNumHashes + 4);
    packCounter++;
    numHashes -= MAX_NUM_HASH;
  }
  return myLL;
}

void parse_data(packet_info* packetinfo, peer* p)
{
  ll* tmplist = create_ll();
  chunk_table* find = NULL;
  uint8_t n = packetinfo->numberHashes[0];
  uint8_t tempChunk[CHUNK];
  bzero(tempChunk, CHUNK);
  int seqNumber;
  int headerLength;
  int totalPacketLength;
  int delno;

  switch (packetinfo->packetType[0]) {

    case 0:
    /*
      // IF WHOHAS
         extract chunk;
         check if chunk exists in has_chunks
         create linked list of request chunks that we have
         call gen_WHOIGET with IHAVE and pass in the linked list
         should return a linked list of packets to send
         use sendto to send the packets
    */
      for (uint8_t i = 0; i < n; i++){
        HASH_FIND(hh, has_chunks, packetinfo->body + CHUNK * i, CHUNK, find);

        if(find) add_node(tmplist, packetinfo->body + CHUNK * i, CHUNK);
      }

      p->tosend = append(gen_WHOIGET(tmplist, 1), p->tosend);
      remove_ll(tmplist);
      break;

    case 1:
      /*
      // IF IHAVE
         extract chunk;
         add chunk to the hash table 'p->has_chunks';
         Lookup chunk in 'get_chunks';
         if that chunk has not yet been requested, change the whohas field to
         identify this peer.
         I'll send a GET to him later.
         I'll put in a timer to retransmit the GET after 5 seconds.
      }
      */
      for (uint8_t i = 0; i < n; i++) {
        find = calloc(1, sizeof(chunk_table));
        memmove(find->chunk, packetinfo->body + CHUNK * i, CHUNK);
        find->id = 0;
        HASH_ADD(hh, p->has_chunks, chunk, CHUNK, find);
      }

      break;

    case 2:
       /* IF GET
       extract the chunk;
       check if we have it;
       If we don't, deny him. eheheh.
       if we do, send it.
       */
      for(uint8_t i = 0; i < n; i++) {
        HASH_FIND(hh, has_chunks, packetinfo->body + CHUNK * i, CHUNK, find);

        if(find) {
          memcpy(tempChunk, packetinfo->body + CHUNK * i, CHUNK);
          p->tosend = append(p->tosend, gen_DATA(tempChunk));
          bzero(tempChunk, CHUNK);

        } else {
          //Generate a denial message.
        }
      }

      break;
      /*
      // IF DATA
         Gonna have to do some flow control logic here;
         extract the data and store it in a 512KB buffer.
         How are we gonna know we recevied the entire chunk of data?
         ANS: use a goddamn bytebuf. when pos = 512KB, we hit gold.
         Perform a checksum afterwards, if badhash, send GET again,
         else write the data into a file using fseek and all.
      */

    case 3:
      //If seq Number != Last Acked + 1, DUPACK
      //Else, copy data into bytebuf
      //Check, complete? If yes, gotcha = True
      //  Perform checksum. If passes, good, else, send GET again
      //Send ACK seqNumber, update Last acked.

      seqNumber = binary2int(packetinfo->sequenceNumber, 4);
      headerLength = binary2int(packetinfo->headerLength, 2);
      totalPacketLength = binary2int(packetinfo->totalPacketLength, 2);

      if((unsigned int)seqNumber != p->LPRecv + 1){
        p->tosend = gen_ACK(p->LPRecv, 2);

      } else {
        HASH_FIND(hh, get_chunks, p->chunk, HASH_SIZE, find);

        // if(find == NULL)
          // Badstuff

        mmemcat(find->data, packetinfo->body, totalPacketLength - headerLength);
        if(find->data->pos == CHUNK_SIZE) { //Complete Chunk!
          find->gotcha = 1;
          p->busy = 0;
          HASH_ADD(hh, has_chunks, chunk, CHUNK, find);
          //Check sum here, resend if necessary
        }
        //Send ACK
        p->LPRecv++;
      }

      break;

    case 4:
      //First, check if this is a DUPACK (use p->LPAcked)
      //If yes, increment dupack counter.
      //  If counter == 3, send approp. packet. Set counter to 0.
      //If not, delete packet from node and send next one.
      //Increment p->LPAcked, p->LPAvail
      //Sliding window stuff.

      if(p->LPAcked == (unsigned int) binary2int(packetinfo->ackNumber, 4))
        {
          p->dupCounter++;

          if(p->dupCounter == 3)
            {
              // Resend the lost packets
            }
        }
      else
        {
          delno = (unsigned int) binary2int(packetinfo->ackNumber, 4) - p->LPAcked;

          for (int i = 0; i < delno; i++)
            {
              delete_node(p->tosend);
            }
        }

      break;

    case 5:
      // Leave this for now...

      break;

    /*
    default:
      //WTF?
    */
  }
}

/* #ifdef TESTING */
/* int main(){ */
/*   uint8_t buf1[200] = {2,3,4,1,1,1,0}; */
/*   uint8_t buf2[200] = {1,3,2,1,1,1,0}; */
/*   uint8_t buf3[200] = {6,5,13,2,1,8,0}; */
/*   //  node* cur = NULL; */

/*   ll* test1 = create_ll(); */
/*   add_node(test1, buf1, 200); //A chunk is 20 bytes, fool */
/*   add_node(test1, buf2, 200); */
/*   add_node(test1, buf3, 200); */
/*   add_node(test1, buf3, 200); */
/*   add_node(test1, buf3, 200); */
/*   add_node(test1, buf3, 200); */
/*   add_node(test1, buf3, 200); */
/*   add_node(test1, buf3, 200); */

/*   gen_WHOIGET(test1, 0); */

/* } */
/* #endif */

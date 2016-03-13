/********************************************************/
/* @file packet.c                                       */
/*                                                      */
/* @brief Library to handle packet creation and parsing */
/*                                                      */
/* @author Malek Anabtawi, Fadhil Abubaker              */
/********************************************************/

/*
 * TODO: We need to retransmit a GET if the peer hasn't responded to us.
 * Maybe use the peer->busy field? Set it only if we get the first DATA
 * packet from that peer?
 *
 * TODO: Timeouts.
 *
 */

#include "packet.h"

/* Globals */
//All of the below appear in peer.c

extern  peer*        peer_list;      // Provided in argv

extern  chunk_table* get_chunks;     // Provided in STDIN
extern  chunk_table* has_chunks;     // Provided in argv
extern  chunk_table* master_chunks;  // Provided in argv

extern size_t       max_conn;          // Provided in argv

extern char*        master_data_file;  // Provided in master_chunks file
extern char*        output_file;       // Provided in STDIN

extern size_t       finished;           // Keep track of completed chunks.

/* @brief Creates a bytebuf struct. The bytebuf's buffer has bufsize bytes allocated.
 *        Note that the bytebuf must be freed outside the function, using delete_bytebuf.
 * @param bufsize: Length of the buf argument.
 */
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

/**************************************************/
/* @brief Free a bytebuf and all of its contents. */
/* @param b - The bytebuf to free                 */
/**************************************************/
void delete_bytebuf(struct byte_buf* b)
{
  free(b->buf);
  free(b);
}

/* @brief Moves <size> bytes, starting from <tempRequest>'s pos argument, into
 *        the argument binaryNumber.
 * @param binaryNumber: Destination buffer. Must be at least <size> bytes.
 * @param tempRequest: Source. Copying from tempRequest->buf
 * @param size: Number of bytes to copy.
 */
void mmemmove(uint8_t *binaryNumber, byte_buf *tempRequest, int size){
  memmove(binaryNumber, tempRequest->buf + tempRequest->pos, size);
  tempRequest->pos += size;
}

/* @brief Concatenates the contents of <binaryNumber> to <tempRequest>'s buf
 *        argument.
 * @param tempRequest: tempRequest->buf is the destination buffer.
 * @param binaryNumber: Source buffer.
 * @param size: Number of bytes to copy from binaryNumber.
 */
void mmemcat(byte_buf *tempRequest, uint8_t *binaryNumber, int size){
  memmove(tempRequest->buf + tempRequest->pos, binaryNumber, size);
  tempRequest->pos += size;
}

/* Sets a byte_buf's contents to its default values. */
void mmemclear(byte_buf *b)
{
  b->pos = 0;
  bzero(b->buf, b->bufsize);
}

/* @brief Converts a given binary number to an int.
 * @param binaryNumber: Number to convert.
 * @param len: Length of the buffer.
 */
int binary2int(uint8_t *buf, int len){
  char temp[2*len];
  bzero(temp, 2*len);
  binary2hex(buf, len, temp);
  int ret = (int)strtol(temp, NULL, 16);
  return ret;
}

/* @brief First converts decimalNumber from decimal to hex. Next, it passes the
 *        result to the given hex2binary function.
 * @param decimalNumber: Number to be converted.
 * @param bytesNeeded: The maximum possible number of hex bytes needed to
 *        represent the number.
 * @param binaryNumber: buffer where the binary number is stored
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


/* @brief Parses a given, complete packet.
 * @param packet: The actual packet to be parsed.
 * @param myPack: A struct that will contain information about the packet.
 */
void parse_packet(uint8_t *packet, packet_info* myPack){

  byte_buf *tempRequest = create_bytebuf(PACKET_LENGTH);
  mmemclear(tempRequest);

  memcpy(tempRequest->buf, packet, PACKET_LENGTH);

  mmemmove(myPack->magicNumber,   tempRequest,     2);
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

  if(packetType == 0 || packetType == 1){

    mmemmove(myPack->numberHashes, tempRequest,    1);
    mmemmove(myPack->padding, tempRequest,         3);
  }

  int headerLength = binary2int(myPack->headerLength, 2);
  int totalPacketLength = binary2int(myPack->totalPacketLength, 2);

  mmemmove(myPack->body, tempRequest, totalPacketLength - headerLength);
  delete_bytebuf(tempRequest);
}

//Test later
ll* gen_ACK(int ackNum, int copies){

  byte_buf *tempRequest = create_bytebuf(PACKET_LENGTH);
  mmemclear(tempRequest);
  ll* myLL = create_ll();

  uint8_t magicNumber[2];
  uint8_t versionNumber[1] = {VERSION_NUMBER};
  uint8_t packetType[1] = {ACK_TYPE};
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
  dec2hex2binary(HEADER, 4, totalPacketLength);
  dec2hex2binary(ackNum, 8, ackNumber);

  mmemcat(tempRequest, magicNumber,       2);
  mmemcat(tempRequest, versionNumber,     1);
  mmemcat(tempRequest, packetType,        1);
  mmemcat(tempRequest, headerLength,      2);
  mmemcat(tempRequest, totalPacketLength, 2);
  mmemcat(tempRequest, sequenceNumber,    4);
  mmemcat(tempRequest, ackNumber,         4);

  while(copies){
    add_node(myLL, tempRequest->buf, HEADER + DATA_LENGTH, 0);
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

  if(!(fseek(fp, id * CHUNK_SIZE, SEEK_SET))){
    //Error!
  }

  if(fread((char *)buf, CHUNK_SIZE, 1, fp) == 0){ //BROKEN?
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

    add_node(myLL, tempRequest->buf, HEADER + DATA_LENGTH, 1);
    packCounter++;
    seqNumber++;
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

    if(packetCode != 2){
      dec2hex2binary(HEADER + CHUNK * tempNumHashes + 4, 4,
                     totalPacketLength);
    } else {
      dec2hex2binary(HEADER + CHUNK * tempNumHashes, 4,
                     totalPacketLength);
    }

    dec2hex2binary(tempNumHashes, 2, numberHashes);

    mmemcat(tempRequest, magicNumber,       2);
    mmemcat(tempRequest, versionNumber,     1);
    mmemcat(tempRequest, packetType,        1);
    mmemcat(tempRequest, headerLength,      2);
    mmemcat(tempRequest, totalPacketLength, 2);
    mmemcat(tempRequest, sequenceNumber,    4);
    mmemcat(tempRequest, ackNumber,         4);
    if(packetCode != 2){
      mmemcat(tempRequest, numberHashes,      1);
      mmemcat(tempRequest, padding,           3);
    }

    while(temp != NULL){
      mmemcat(tempRequest, temp->data, CHUNK); // No
      temp = temp->next;
    }

    if(packetCode != 2)
      add_node(myLL, tempRequest->buf, HEADER + CHUNK * tempNumHashes + 4, 0);
    else
      add_node(myLL, tempRequest->buf, HEADER + CHUNK * tempNumHashes, 2);

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
  unsigned int seqNumber; unsigned int ackNumber;
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

      if(find) add_node(tmplist, packetinfo->body + CHUNK * i, CHUNK, 0);
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
    n = 1;
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

    seqNumber = (unsigned int) binary2int(packetinfo->sequenceNumber, 4);
    headerLength = binary2int(packetinfo->headerLength, 2);
    totalPacketLength = binary2int(packetinfo->totalPacketLength, 2);

    /* We received a DATA packet, check if there's a GET in front of
     * the send queue. If there is, delete it. */
    /* if(p->tosend && p->tosend->first && p->tosend->first->type == 2) */
    /*   { */
    /*     delete_node(p->tosend); */
    /*   } */

    /* Check if we received a packet we got before */
    if (seqNumber <= p->LPRecv) {
      p->tosend = append(gen_ACK(p->LPRecv, 1), p->tosend);
      break;
    }

    /* Check if we received an inorder packet  */
    if(seqNumber > p->LPRecv + 1) {
      p->tosend = append(gen_ACK(p->LPRecv, 3), p->tosend);
      break;
    }
    /* We received the next expected packet */
    else {
      HASH_FIND(hh, get_chunks, p->chunk, HASH_SIZE, find);

      // if(find == NULL)
      // Badstuff

      mmemcat(find->data, packetinfo->body, totalPacketLength - headerLength);

      p->LPRecv = seqNumber;

      if(find->data->pos == CHUNK_SIZE) { //Complete Chunk!
        find->gotcha = 1;
        p->busy = 0;
        p->LPRecv = 0; // Reset state.

        /* copying is to ensure consistency within the Hash Table lib */
        chunk_table* copy = duptable(find);

        HASH_ADD(hh, has_chunks, chunk, CHUNK, copy);
        // Check sum here, resend if necessary
        // Save to file;
        save2file(find);
        finished++;
      }
      //Send ACK
      p->tosend = append(gen_ACK(seqNumber, 1), p->tosend);
    }

    break;

  case 4:
    //First, check if this is a DUPACK (use p->LPAcked)
    //If yes, increment dupack counter.
    //  If counter == 3, send approp. packet. Set counter to 0.
    //If not, delete packet from node and send next one.
    //Increment p->LPAcked, p->LPAvail
    //Sliding window stuff.
    ackNumber = (unsigned int) binary2int(packetinfo->ackNumber, 4);

    if(p->LPAcked == ackNumber)
      {
        p->dupCounter++;

        if(p->dupCounter >= 3)
          {
            /* Resend the lost packets */
            /* I guess this happens automatically
             * because I don't delete any nodes? */
          }
      }
    else
      {
        p->dupCounter = 0;
        delno = ackNumber - p->LPAcked;

        for (int i = 0; i < delno; i++)
          {
            delete_node(p->tosend);
          }

        p->LPAcked = ackNumber;
        p->LPAvail = p->LPAcked + 8;
      }

    if(p->LPAcked == 512) // We send 512 packets of a 1000 bytes each.
      {
        // Reset the sliding window state for this peer.
        p->LPAcked = 0;
        p->LPSent  = 0;
        p->LPAvail = 8;
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

void save2file(chunk_table* chunk)
{
  FILE* fp;

  fp = fopen(output_file, "a");

  fseek(fp, chunk->id * CHUNK_SIZE, SEEK_SET);

  fwrite(chunk->data->buf, CHUNK_SIZE, 1, fp);

  fclose(fp);
}

/***********************************************************************/
/* @brief A debugging utility that prints packet information to STDIN. */
/* @param packet - The packet to print.                                */
/***********************************************************************/
void print_packet(uint8_t* packet, int i)
{
  packet_info myPack = {0};
  byte_buf *tempRequest = create_bytebuf(PACKET_LENGTH);
  mmemclear(tempRequest);

  memcpy(tempRequest->buf, packet, PACKET_LENGTH);

  mmemmove(myPack.magicNumber,        tempRequest, 2);
  mmemmove(myPack.versionNumber,      tempRequest, 1);
  mmemmove(myPack.packetType,         tempRequest, 1);
  mmemmove(myPack.headerLength,       tempRequest, 2);
  mmemmove(myPack.totalPacketLength,  tempRequest, 2);
  mmemmove(myPack.sequenceNumber,     tempRequest, 4);
  mmemmove(myPack.ackNumber,          tempRequest, 4);

  int type = binary2int(myPack.packetType, 1);
  int hlen = binary2int(myPack.headerLength,      2);
  int plen = binary2int(myPack.totalPacketLength, 2);

  if(type == 0 || type == 1){
    mmemmove(myPack.numberHashes, tempRequest,    1);
    mmemmove(myPack.padding, tempRequest,         3);
  }

  mmemmove(myPack.body, tempRequest, plen - hlen);

  int numh = binary2int(myPack.numberHashes, 1);

  char typestr[10];

  switch (type)
    {
    case 0:
      sprintf(typestr, "WHOHAS");
      break;

    case 1:
      sprintf(typestr, "IHAVE");
      break;

    case 2:
      sprintf(typestr, "GET");
      break;

    case 3:
      sprintf(typestr, "DATA");
      break;

    case 4:
      sprintf(typestr, "ACK");
      break;

    case 5:
      sprintf(typestr, "DENIED");
      break;

    default:
      sprintf(typestr, "DEFAULT");
      break;

    }

  //mmemmove(myPack.body, tempRequest, plen - hlen);

  printf("\n###############################\n");
  if(i == SEND) printf("Sending packet with contents:\n");
  if(i == RECV) printf("Receiving packet with contents:\n");
  printf("Magic   Number: %d\n", binary2int(myPack.magicNumber, 2));
  printf("Version Number: %d\n", binary2int(myPack.versionNumber, 1));
  printf("Packet  Type  : %s\n", typestr);
  printf("Header  Len   : %d\n", hlen);
  printf("Packet  Len   : %d\n", plen);
  printf("Seq     No    : %d\n", binary2int(myPack.sequenceNumber , 4));
  printf("Ack     No    : %d\n", binary2int(myPack.ackNumber , 4));
  printf("Hash    No    : %d\n", numh);
  //printf("Body          : %d\n", binary2int(myPack.numberHashes , 4));

  char hash[45] = {0};

  for(int i = 0; i < numh; i++)
    {
      binary2hex(myPack.body + 20 * i, 20, hash);
      printf("Hash %d: %s \n", i, hash);
      bzero(hash, 45);
    }

  if(type == 2) // GET
    {
      binary2hex(myPack.body, 20, hash);
      printf("Hash %d: %s \n", i, hash);
      bzero(hash, 45);
    }

  printf("###############################\n\n");

  delete_bytebuf(tempRequest);
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

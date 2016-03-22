#ifndef PEER_H
#define PEER_H

#include <sys/types.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "debug.h"
#include "spiffy.h"
#include "chunk.h"
#include "bt_parse.h"
#include "input_buffer.h"
#include "uthash.h"

#include "packet.h"

/* Macros */

#define HASH_SIZE     20
#define PORT_LEN      10
#define PEER_KEY_LEN  INET_ADDRSTRLEN + 5 + PORT_LEN

#define SEND          0
#define RECV          1

typedef struct chunk_table chunk_table;
typedef struct peer peer;

/* Structs */

struct chunk_table {

  uint8_t        chunk[HASH_SIZE];  // Key
  size_t         id;

  /* Fields below are to be only used for get_chunks */

  struct byte_buf*       data;      // Store the data represented by the chunk here
  int                    requested; // Has this chunk been 'GET'ed?
  int                    gotcha;    // Have we received this chunk (fully)?

  /* Format:     "address:port" */
  char            whohas[PEER_KEY_LEN]; // The peer from who to 'GET'

  UT_hash_handle hh;

};


struct peer {

  int                id;

  /* Format:         "address:port" */
  char               key[PEER_KEY_LEN];
  struct sockaddr_in addr;

  chunk_table*       has_chunks;
  struct byte_buf*   buf;
  ll*                tosend;              // list of packets to send to this peer
  int                busy;                // Are we requesting DATA from this peer?
  int                needy;               // Are we sending DATA to this peer?
  uint8_t            chunk[HASH_SIZE];    // identifies the chunk being obtained from this peer
                                          // sent by this peer.

  /* Flow control/reliability state.  Note: use only for DATA/ACK packets */

  unsigned int       LPAcked;     //Last packet Acked
  unsigned int       LPSent;      //Last packet sent
  unsigned int       LPAvail;     //Last packet available (maximum packet able to send)
  unsigned int       LPRecv;      //Last packet received
  int                dupCounter;  //Checks if we have a DUP'd ACK

  struct timespec    start_time;
  int                ttl;

  /* Congestion control */

  int                window;
  int                rttcnt;
  int                ssthresh;
  struct timespec    rtt;

  UT_hash_handle     hh;

};


/* Prototypes */

void peer_run(bt_config_t *config);
void process_inbound_udp(int sock);
void process_get(char *chunkfile, char *outputfile);
void handle_user_input(char *line, void *cbdata);
  struct timespec    rtt;

  UT_hash_handle     hh;

void global_populate(bt_config_t* config);
void convert_LL2HT(bt_peer_t* ll_peers, peer** ht_peers, short myid);
void make_chunktable(char* chunk_file, chunk_table** table, int flag);
void sliding_send(peer* p, int sock);
void choose_peer();
chunk_table* duptable(chunk_table* src);

void clean_table(chunk_table* table);
void choose_another(peer* bad);
void gen_graph(peer *p);
void computeRTT(peer *p);
#endif

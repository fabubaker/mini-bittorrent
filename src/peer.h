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

/* Global variables that keep the state of this peer */

peer*        peer_list = NULL;      // Provided in argv

chunk_table* get_chunks = NULL;     // Provided in STDIN
chunk_table* has_chunks = NULL;     // Provided in argv
chunk_table* master_chunks = NULL;  // Provided in argv

size_t       max_conn;              // Provided in argv

char*        master_data_file;      // Provided in master_chunks file
char*        output_file;           // Provided in STDIN

/* Structs */

typedef struct chunk_table {

  uint8_t        chunk[HASH_SIZE];  // Key
  size_t         id;
  UT_hash_handle hh;

} chunk_table;

typedef struct peer {

  int                id;
  int                timeoutfd;

  /* Format:         "address:port" */
  char               key[PEER_KEY_LEN];
  struct sockaddr_in addr;

  chunk_table*       has_chunks;
  UT_hash_handle     hh;

} peer;


/* Prototypes */

void peer_run(bt_config_t *config);
void process_inbound_udp(int sock);
void process_get(char *chunkfile, char *outputfile);
void handle_user_input(char *line, void *cbdata);


#endif

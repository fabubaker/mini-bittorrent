/*******************************************************************/
/* @file peer.c                                                    */
/*                                                                 */
/* @brief A simple peer implementation of the BitTorrent protocol. */
/*                                                                 */
/* @author Fadhil Abubaker, Malek Anabtawi                         */
/*******************************************************************/

#include "peer.h"

/* Globals */

peer*        peer_list = NULL;      // Provided in argv

chunk_table* get_chunks = NULL;     // Provided in STDIN
chunk_table* has_chunks = NULL;     // Provided in argv
chunk_table* master_chunks = NULL;  // Provided in argv

size_t       max_conn;              // Provided in argv

char*        master_data_file;      // Provided in master_chunks file
char*        output_file;           // Provided in STDIN

size_t       finished = 0;          // Keep track of completed chunks.

size_t       debugcount = 0;

/* Definitions */

int main(int argc, char **argv)
{
  bt_config_t config;

  bt_init(&config, argc, argv);

  DPRINTF(DEBUG_INIT, "peer.c main beginning\n");

#ifdef TESTING
  config.identity = 1; // your group number here
  strcpy(config.chunk_file, "chunkfile");
  strcpy(config.has_chunk_file, "haschunks");
#endif

  /* Parse the cmd line tokens */
  bt_parse_command_line(&config);

  /* Populate the global variables */
  global_populate(&config);

#ifdef DEBUG
  if (debug & DEBUG_INIT) {
    bt_dump_config(&config);
  }
#endif

  peer_run(&config);
  return 0;
}

/**************************************************************/
/* @brief Given a config struct, begin operating the BT peer. */
/* @param config The config file of the peer.                 */
/**************************************************************/
void peer_run(bt_config_t *config) {
  int sock;
  struct sockaddr_in myaddr;
  fd_set readfds;
  struct user_iobuf *userbuf;

  /* Create a buffer to handle user inputs */
  if ((userbuf = create_userbuf(USERBUF_SIZE)) == NULL) {
    perror("peer_run could not allocate userbuf");
    exit(-1);
  }

  if ((sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP)) == -1) {
    perror("peer_run could not create socket");
    exit(-1);
  }

  bzero(&myaddr, sizeof(myaddr));
  myaddr.sin_family = AF_INET;
  myaddr.sin_addr.s_addr = htonl(INADDR_ANY);
  myaddr.sin_port = htons(config->myport);

  if (bind(sock, (struct sockaddr *) &myaddr, sizeof(myaddr)) == -1) {
    perror("peer_run could not bind socket");
    exit(-1);
  }

  //  spiffy_init(config->identity, (struct sockaddr *)&myaddr, sizeof(myaddr));

  struct timeval tv;
  tv.tv_sec  = 3; // 3 seconds in the beginning
  tv.tv_usec = 0;

  while (1) {
    int nfds;
    peer* p; peer* tmp;

    FD_ZERO(&readfds);
    FD_SET(STDIN_FILENO, &readfds);
    FD_SET(sock, &readfds);

    nfds = select(sock+1, &readfds, NULL, NULL, &tv);

    if (nfds > 0) {
      if (FD_ISSET(sock, &readfds))
        {
          process_inbound_udp(sock);
        }

      if (FD_ISSET(STDIN_FILENO, &readfds))
        {
          process_user_input(STDIN_FILENO, userbuf, handle_user_input,
                             "Currently unused");
        }
    }

    /* Decide which peer to get which chunk from */
    choose_peer();

    /* Loop through all peers and send packets to them using
     * a sliding window protocol. */
    HASH_ITER(hh, peer_list, p, tmp) {
      sliding_send(p, sock);
    }

    /* Insert code here to update 'tv' */
    tv.tv_sec = 3; // 3 seconds
    tv.tv_usec = 0;
  }
}

/*
@brief Process incoming UDP data and store it in the peer state
*/
void process_inbound_udp(int sock) {
#define BUFLEN 1500
  peer* find;
  struct sockaddr_in from;
  socklen_t fromlen;
  uint8_t buf[BUFLEN];
  char    keybuf[PEER_KEY_LEN];

  bzero(buf, BUFLEN);
  bzero(keybuf, PEER_KEY_LEN);

  fromlen = sizeof(from);
  recvfrom(sock, buf, BUFLEN, 0, (struct sockaddr *) &from, &fromlen);

  sprintf(keybuf, "%s:%d",
          inet_ntoa(from.sin_addr),
          ntohs(from.sin_port));

  HASH_FIND_STR( peer_list, keybuf, find );

  if(find == NULL)
    {
      printf("We have problem! \n");
      exit(0);
    }

  /* Save it in the peer state to be processed later */
  mmemclear(find->buf);
  mmemcat(find->buf, buf, BUFLEN);

  printf("Incoming message from %s:%d\n\n",
         inet_ntoa(from.sin_addr),
         ntohs(from.sin_port));

#ifdef DEBUG
  if (debug & DEBUG_SOCKETS)
    {
      print_packet(buf, RECV);
    }
#endif

  packet_info packetinfo;
  bzero(&packetinfo, sizeof(packetinfo));

  /* Store all packet information in the struct below */
  parse_packet(find->buf->buf, &packetinfo);

  /* If there is data, parse it and create
   * a linked list of packets to be sent to
   * that peer.
   */
  parse_data(&packetinfo, find);

  /* Everytime a chunk has been fully received,
   * delete it from the hash table, but only
   * after writing its data to the file.
   * Use HASH_COUNT to determine if there are any chunks
   * left to be received.
   */
  if(HASH_COUNT(get_chunks) == finished && get_chunks != NULL)
    {
      printf("GOT \n");
    }
}

void process_get(char *chunkfile, char *outputfile) {
  peer* find; chunk_table* find2;
  peer* tmp ; chunk_table* tmp2;  ll* llget = create_ll();

  output_file = malloc(strlen(outputfile));
  strcpy(output_file, outputfile);

  make_chunktable(chunkfile, &get_chunks, 2);

  /* Iterate through all the chunks to get and create a linked list */
  HASH_ITER(hh, get_chunks, find2, tmp2) {
    add_node(llget, find2->chunk, HASH_SIZE, 0);
  }

  /* Iterate through all peers and generate WHOHAS for each of them */
  HASH_ITER(hh, peer_list, find, tmp) {
    find->tosend = append(gen_WHOIGET(llget, 0), find->tosend);
  }

  remove_ll(llget);
}

void handle_user_input(char *line, void *cbdata) {
  char chunkf[128], outf[128];
  cbdata++; // Appease the compiler

  bzero(chunkf, sizeof(chunkf));
  bzero(outf, sizeof(outf));

  if (sscanf(line, "GET %120s %120s", chunkf, outf)) {
    if (strlen(outf) > 0) {
      process_get(chunkf, outf);
    }
  }
}

void global_populate(bt_config_t* config)
{
  FILE* file;
  char* buf = NULL; size_t n = 0;
  ssize_t len = 0;

  /* Make max_conn a global variable */
  max_conn = config->max_conn;

  /* Get the path to the master data file  */
  file =  fopen(config->chunk_file, "r");
  len = getline(&buf, &n, file);  // Need to free buf...

  master_data_file = malloc(len);
  sscanf(buf, "File: %s", master_data_file);
  free(buf);

  /* Convert peers from linked list to hash table   */
  convert_LL2HT(config->peers, &peer_list, config->identity);

  /* Open the master_chunk file and make a hash table out of it  */
  make_chunktable(config->chunk_file, &master_chunks, 0);

  /* Open the has_chunks file and make a hash table out of it  */
  make_chunktable(config->has_chunk_file, &has_chunks, 1);
}


/**************************************************************************/
/* @brief Takes a linked list containing all the peers and converts       */
/*        it into a hash table with the string "address:port" as the key. */
/* @param ll_peers The linked list of peers                               */
/* @param ht_peers The hash table of peers                                */
/**************************************************************************/
void convert_LL2HT(bt_peer_t* ll_peers, peer** ht_peers, short myid)
{
  bt_peer_t *cur = NULL;
  peer* tmppeer  = NULL;

  for(cur = ll_peers; cur; cur = cur->next)
    {
      if(cur->id == myid)
        continue;

      tmppeer = calloc(1, sizeof(peer));

      tmppeer->id = cur->id;

      sprintf(tmppeer->key, "%s:%d",
              inet_ntoa(cur->addr.sin_addr),
              ntohs(cur->addr.sin_port));

      tmppeer->addr.sin_addr.s_addr = cur->addr.sin_addr.s_addr;
      tmppeer->addr.sin_family = AF_INET;
      tmppeer->addr.sin_port   = cur->addr.sin_port;

      tmppeer->has_chunks = NULL;
      tmppeer->buf        = create_bytebuf(PACKET_LENGTH);
      tmppeer->tosend     = NULL;
      tmppeer->busy       = 0;
      bzero(tmppeer->chunk, HASH_SIZE);

      tmppeer->LPAcked    = 0;
      tmppeer->LPSent     = 0;
      tmppeer->LPAvail    = 8;
      tmppeer->LPRecv     = 0;

      tmppeer->dupCounter = 0;

      HASH_ADD_STR( *ht_peers, key, tmppeer );
    }
}



/*********************************************************************************/
/* @brief Takes a file containing lines with <id chunk-hash> and stores          */
/*        them in a hash table, with the hashes as key.                          */
/* @param chunk_file The file containing the <id chunk-hash> lines.              */
/* @param table      The table to populate                                       */
/* @param flag       To indicate a master-chunk-file or a get-chunk-file or not. */
/*********************************************************************************/

void make_chunktable(char* chunk_file, chunk_table** table, int flag)
{
  FILE* file;
  char* buf = calloc(1, 3*HASH_SIZE);
  size_t n = 3*HASH_SIZE;
  ssize_t len = 0;
  chunk_table* tmptable = NULL;

  file = fopen(chunk_file, "r");

  if(!flag)
    {
      getline(&buf, &n, file); // Remove "File: ..."
      getline(&buf, &n, file); // Remove "Chunks: ..."
      bzero(buf, n);
    }

  while((len = getline(&buf, &n, file)) != -1)
    {
      char tmpbuf[HASH_SIZE*2] = {0};

      tmptable = calloc(1,sizeof(chunk_table));
      sscanf(buf, "%zu %s", &(tmptable->id), tmpbuf);
      ascii2hex(tmpbuf, HASH_SIZE*2, tmptable->chunk);

      if(flag == 2)
        tmptable->data = create_bytebuf(CHUNK_SIZE); // To store incoming DATA packs
      else
        tmptable->data = NULL;

      tmptable->requested = 0;
      tmptable->gotcha =  0;

      bzero(tmptable->whohas, PEER_KEY_LEN);

      HASH_ADD(hh, *table, chunk, HASH_SIZE, tmptable);
    }

  free(buf);
  return;
}

/***************************************************************************/
/* @brief Sends packets to peer p according to the sliding window protocol */
/*        state.                                                           */
/* @p     The peer state.                                                  */
/***************************************************************************/
void sliding_send(peer* p, int sock)
{
  node* cur;
  //struct timespec now;

  if(!p->tosend)
    return;

  //clock_gettime(CLOCK_MONOTONIC, &now);

  /* long long unsigned int diff = */
  /*   1000 * (now.tv_sec - p->start_time.tv_sec) + */
  /*   (now.tv_nsec - p->start_time.tv_nsec) / 1000000; // Convert to ms */

  /* Check if this peer timed out. */
  /* if(diff > 3000) // ms */
  /*   /\* Timed out, we need to reset packets *\/ */
  /*   p->LPSent = p->LPAcked; */

  cur = p->tosend->first;

  while(cur != NULL)
    {
      if(cur->type) // DATA packet
        {
          if(p->LPSent < p->LPAvail)
            {
              sendto(sock, cur->data, DATA_SIZE, 0,
                     (struct sockaddr*)(&p->addr), sizeof(p->addr));
              //clock_gettime(CLOCK_MONOTONIC, &p->start_time);

#ifdef DEBUG
              if (debug & DEBUG_SOCKETS)
                {
                  print_packet(cur->data, SEND);
                }
#endif

              p->LPSent++;
            }
          cur = cur->next;
        }
      else        // Any other packet
        {
          sendto(sock, cur->data, DATA_SIZE, 0,
                 (struct sockaddr*)(&p->addr), sizeof(p->addr));

#ifdef DEBUG
          if (debug & DEBUG_SOCKETS)
            {
              print_packet(cur->data, SEND);
            }
#endif

          delete_node(p->tosend);
          cur = p->tosend->first;
        }
    }
}


void choose_peer()
{
  peer* p; peer* tmp;
  chunk_table* find; chunk_table* tmp2;
  chunk_table* find2;

  /* If the user hasn't requested anything, exit immediately */
  if (!get_chunks)
    return;

  ll* llget = create_ll();

  /* Iterate through each peer to decide which chunk to get */
  HASH_ITER(hh, peer_list, p, tmp) {

    if(!p->has_chunks) /* No IHAVE from this peer, continue  */
      continue;

    if(p->busy) /* Leave him alone */
      continue;

    /* Iterate through each peer's chunks */
    HASH_ITER(hh, p->has_chunks, find, tmp2) {

      HASH_FIND(hh, get_chunks, find->chunk, HASH_SIZE, find2);

      if(!find2 || !find2->requested)
        {
          find2->requested = 1;

          add_node(llget, find2->chunk, HASH_SIZE, 0);
          p->tosend = append(gen_WHOIGET(llget, 2), p->tosend);

          p->busy = 1;

          bzero(p->chunk, HASH_SIZE);
          memcpy(p->chunk, find2->chunk, HASH_SIZE);

          delete_node(llget);

          break;
        }
    }
  }

  remove_ll(llget);
}

chunk_table* duptable(chunk_table* src)
{
  chunk_table* dst;

  dst = malloc(sizeof(chunk_table));

  memcpy(dst->chunk, src->chunk, HASH_SIZE);

  dst->id = src->id;

  return dst;
}

int HT_count(chunk_table* table)
{
  return HASH_COUNT(table);
}
/*
  Notes:

  The other tactic (much more sane) is to avoid the mess with signals and
  setitimer completely. setitimer is seriously legacy and causes problems for
  all sorts of things (eg. it can cause functions like getaddrinfo to hang, a
  bug that still hasn't been fixed in glibc
  (http://www.cygwin.org/frysk/bugzilla/show_bug.cgi?id=15819). Signals are bad
  for your health. So the "normal" tactic is to use the timeout argument to
  select. You have a linked list of timers, objects you use to manager periodic
  events in your code. When you call select, you use as the timeout the shortest
  of your remaining timers. When the select call returns, you check if any
  timers are expired and call the timer handler as well as the handlers for your
  fd events. That's a standard application event loop. This way your loop code
  so you can listen for timer-driven events as well as fd-driven events. Pretty
  much every application on your system uses some variant on this mechanism.

  Use signalfd

*/

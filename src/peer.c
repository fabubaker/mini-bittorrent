/*******************************************************************/
/* @file peer.c                                                    */
/*                                                                 */
/* @brief A simple peer implementation of the BitTorrent protocol. */
/*                                                                 */
/* @author Fadhil Abubaker, Malek Anabtawi                         */
/*******************************************************************/

#include "peer.h"

/*************************************
  Congestion Control: To-Do
  *************************************

TODO: computeRTT(peer* p); check

This function take in a peer and a new round trip time and
computes the a new estimated rtt, using:

EstimatedRTT = alpha * EstimatedRTT + (1 - alpha) * SampleRTT
,where alpha is between 0.8 to 0.9.

TODO: Modify sliding_send: every time a peer timed out, recalculate
congestion window size.

TODO: Modify parse_data: everytime we get an ACK, adjust congestion window
depending on ssthresh or not.

TODO: Graphing window size.
Print:
-ID of the connection
-time in ms since peer has been running
-window size

*/

/* Globals */

peer*        peer_list = NULL;      // Provided in argv

chunk_table* get_chunks = NULL;     // Provided in STDIN
chunk_table* has_chunks = NULL;     // Provided in argv
chunk_table* master_chunks = NULL;  // Provided in argv

size_t       max_conn;              // Provided in argv

char*        master_data_file;      // Provided in master_chunks file
char*        output_file;           // Provided in STDIN
char*        get_chunk_file;        // Provided in STDIN

size_t       finished = 0;          // Keep track of completed chunks.

size_t       debugcount = 0;

struct timespec    inception;
static const char *graph_file = "problem2-peer.txt";
FILE *graphFP;

/* Definitions */

int main(int argc, char **argv)
{
  graphFP = fopen(graph_file, "w");
  bt_config_t config;
  clock_gettime(CLOCK_MONOTONIC, &inception);
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

void gen_graph(peer *p){
  char line[512];
  bzero(line, 512);
  struct timespec current;
  clock_gettime(CLOCK_MONOTONIC, &current);

  unsigned long long int time_since =
    1000 * (current.tv_sec - inception.tv_sec) +
    (current.tv_nsec - inception.tv_nsec) / 1000000;

  sprintf(line, "%d\t%llu\t%d\n", p->id, time_since, p->window);
  fflush(graphFP);
}

//Compute RTT.
void computeRTT(peer *p){
  struct timespec current;
  double alpha = 0.875;
  clock_gettime(CLOCK_MONOTONIC, &current);

  long long unsigned int sample =
    1000 * (current.tv_sec - p->start_time.tv_sec) +
    (current.tv_nsec - p->start_time.tv_nsec) / 1000000;

  long long unsigned int expected =
    1000 * (p->rtt.tv_sec) + (p->rtt.tv_nsec) / 1000000;

  long long unsigned int rttVal = alpha * expected + (1 - alpha) * sample;

  p->rtt.tv_sec = rttVal / 1000;
  p->rtt.tv_nsec = (rttVal % 1000) * 1000000;
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
  //myaddr.sin_addr.s_addr = htonl(INADDR_ANY);
  inet_aton("127.0.0.1", (struct in_addr *)&myaddr.sin_addr.s_addr);
  myaddr.sin_port = htons(config->myport);

  if (bind(sock, (struct sockaddr *) &myaddr, sizeof(myaddr)) == -1) {
    perror("peer_run could not bind socket");
    exit(-1);
  }

  spiffy_init(config->identity, (struct sockaddr *)&myaddr, sizeof(myaddr));

  struct timeval tv;
  tv.tv_sec  = 0;
  tv.tv_usec = 500000; // half a second

  while (1) {
    int nfds;
    peer* p; peer* tmp;
    struct timespec max;

    max.tv_sec = 0;
    max.tv_nsec = 0;

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
    if(get_chunks)
      choose_peer();

    /* Loop through all peers and send packets to them using
     * a sliding window protocol. */
    HASH_ITER(hh, peer_list, p, tmp) {
      sliding_send(p, sock);

      if(p->rtt.tv_sec >= max.tv_sec &&
         p->rtt.tv_nsec >= max.tv_nsec)
        {
          max.tv_sec  = p->rtt.tv_sec;
          max.tv_nsec = p->rtt.tv_nsec;
        }
    }

    /* Insert code here to update 'tv' */
    tv.tv_sec  = max.tv_sec;
    tv.tv_usec = max.tv_nsec/1000;
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
  spiffy_recvfrom(sock, buf, BUFLEN, 0, (struct sockaddr *) &from, &fromlen);

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

  /* Reset timeout state */
  clock_gettime(CLOCK_MONOTONIC, &find->start_time);
  find->ttl = 0;

  printf("Incoming message from %s:%d\n",
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
      printf("GOT %s\n", get_chunk_file);
      clean_table(get_chunks);
      get_chunks = NULL;

      free(get_chunk_file);
      get_chunk_file = NULL;

      free(output_file);
      output_file = NULL;

      finished = 0;
    }
}

void process_get(char *chunkfile, char *outputfile) {
  peer* find; chunk_table* find2;
  peer* tmp ; chunk_table* tmp2;  ll* llget = create_ll();
  long long unsigned filesize = 0;

  output_file = calloc(1, strlen(outputfile)+1);
  strcpy(output_file, outputfile);

  get_chunk_file = calloc(1, strlen(chunkfile)+1);
  strcpy(get_chunk_file, chunkfile);

  make_chunktable(chunkfile, &get_chunks, 2);

  filesize = CHUNK_SIZE * HASH_COUNT(get_chunks);
  FILE *fp = fopen(outputfile, "w");
  ftruncate(fileno(fp), filesize);
  fclose(fp);

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
      tmppeer->needy      = 0;
      bzero(tmppeer->chunk, HASH_SIZE);

      tmppeer->LPAcked    = 0;
      tmppeer->LPSent     = 0;
      tmppeer->LPAvail    = 8;
      tmppeer->LPRecv     = 0;

      tmppeer->dupCounter = 0;

      bzero(&(tmppeer->start_time), sizeof(tmppeer->start_time));
      tmppeer->ttl = 0;

      tmppeer->window   = 1;
      tmppeer->ssthresh = 64;

      tmppeer->rtt.tv_sec = 0;
      tmppeer->rtt.tv_nsec = 250000;

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
  struct timespec now;

  if(!p->tosend)
    return;

  clock_gettime(CLOCK_MONOTONIC, &now);

  long long unsigned int diff =
    1000 * (now.tv_sec - p->start_time.tv_sec) +
    (now.tv_nsec - p->start_time.tv_nsec) / 1000000; // Convert to ms

  //  Check if this peer timed out.
  if(diff > 500) // ms
    {
      p->ttl++;

      if(p->ttl == 5) // DEAD
        {
          if(p->busy) /* I am receiving DATA from this peer */
            {
              choose_another(p); /* Choose some other peer */
            }

          /* Free up this peer's resources, he's dead lol */
          remove_ll(p->tosend);
          p->tosend = NULL;

          clean_table(p->has_chunks);
          p->has_chunks = NULL;

          bzero(&(p->start_time), sizeof(p->start_time));
          bzero(p->chunk, HASH_SIZE);

          p->LPAcked    = 0;
          p->LPSent     = 0;
          p->LPAvail    = 8;
          p->LPRecv     = 0;
          p->dupCounter = 0;
          p->busy       = 0;
          p->needy      = 0;
          p->ttl        = 0;
          p->window     = 1;
          p->ssthresh   = 64;
          p->rtt.tv_sec = 0;
          p->rtt.tv_nsec= 250000;
          return;
        }

      /* Calculate new ssthresh */
      p->ssthresh = p->window / 2;
      if(p->ssthresh < 2) p->ssthresh = 2;
      p->window = 1;

      /* Timed out, we need to resend packets */
      p->LPSent = p->LPAcked;

      /* OR, we may need to resend GET */
      if(p->busy && p->LPRecv == 0)
        {
          ll* llget = create_ll();
          add_node(llget, p->chunk, HASH_SIZE, 0);
          p->tosend = append(gen_WHOIGET(llget, 2), p->tosend);
          free(llget);
        }
    }

  cur = p->tosend->first;

  while(cur != NULL)
    {
      if(cur->type == 1) // DATA packet
        {
          if(p->LPSent < p->LPAvail)
            {
              spiffy_sendto(sock, cur->data, DATA_SIZE, 0,
                     (struct sockaddr*)(&p->addr), sizeof(p->addr));
              clock_gettime(CLOCK_MONOTONIC, &p->start_time);

#ifdef DEBUG
              printf("Outgoing message to %s:%d\n",
                     inet_ntoa(p->addr.sin_addr),
                     ntohs(p->addr.sin_port));

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
          spiffy_sendto(sock, cur->data, DATA_SIZE, 0,
                 (struct sockaddr*)(&p->addr), sizeof(p->addr));

#ifdef DEBUG
          printf("Outgoing message to %s:%d\n",
                 inet_ntoa(p->addr.sin_addr),
                 ntohs(p->addr.sin_port));

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

    /* If a non-GET, non-DATA packet is in queue,
     * try some other time */
    /* if(p->tosend && p->tosend->first && p->tosend->first->type == 0) */
    /*   continue; */

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

void clean_table(chunk_table* table)
{
  chunk_table *cur, *tmp;

  HASH_ITER(hh, table, cur, tmp)
    {
      HASH_DEL(table, cur); /* Delete current entry */

      /* Free memory */
      if(cur->data)
        {
          delete_bytebuf(cur->data);
        }
      free(cur);
    }
}

void choose_another(peer* bad)
{
  chunk_table* find;
  uint8_t      chunk[HASH_SIZE];

  memcpy(chunk, bad->chunk, HASH_SIZE);

  HASH_FIND(hh, get_chunks, chunk, HASH_SIZE, find);

  mmemclear(find->data);
  find->requested = 0;
  find->gotcha    = 0;

  bzero(find->whohas, PEER_KEY_LEN);

  /* choose_peer() will automatically select another peer */
}

/*******************************************************************/
/* @file peer.c                                                    */
/*                                                                 */
/* @brief A simple peer implementation of the BitTorrent protocol. */
/*                                                                 */
/* @author Fadhil Abubaker, Malek Anabtawi                         */
/*******************************************************************/

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
  if ((userbuf = create_userbuf()) == NULL) {
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

  while (1) {
    int nfds;
    FD_SET(STDIN_FILENO, &readfds);
    FD_SET(sock, &readfds);

    nfds = select(sock+1, &readfds, NULL, NULL, NULL);

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
  }
}

void process_inbound_udp(int sock) {
#define BUFLEN 1500
  struct sockaddr_in from;
  socklen_t fromlen;
  char buf[BUFLEN];

  fromlen = sizeof(from);
  spiffy_recvfrom(sock, buf, BUFLEN, 0, (struct sockaddr *) &from, &fromlen);

  printf("PROCESS_INBOUND_UDP SKELETON -- replace!\n"
         "Incoming message from %s:%d\n%s\n\n",
         inet_ntoa(from.sin_addr),
         ntohs(from.sin_port),
         buf);
}

void process_get(char *chunkfile, char *outputfile) {
  printf("PROCESS GET SKELETON CODE CALLED.  Fill me in!  (%s, %s)\n",
         chunkfile, outputfile);
}

void handle_user_input(char *line, void *cbdata) {
  char chunkf[128], outf[128];

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
  char* buf = NULL; int n = 0;
  ssize_t len = 0;

  /* Make max_conn a global variable */
  max_conn = config->max_conn;

  /* Get the path to the master data file  */
  file =  fopen(config->chunk_file, r);
  len = getline(&buf, &n, file);  // Need to free buf...

  master_data_file = malloc(len);
  sscanf(buf, "File: %s", master_data_file);
  free(buf);

  /* Convert peers from linked list to hash table   */
  convert_LL2HT(config->peers, &peer_list);

  /* Open the master_chunk file and make a hash table out of it  */
  make_chunktable(config->chunk_file, &master_chunks, 0);

  /* Open the has_chunks file and make a hash table out of it  */
  make_chunktable(config->has_chunk_file, &has_chunks, 1);

}

void convert_LL2HT(bt_peer_s* peers, peer** peer_list)
{

}

void make_chunktable(char* chunk_file, chunk_table** table, int flag)
{
  FILE* file;
  char buf[3*HASH_SIZE] = {0}; int n = 3*HASH_SIZE;

  ssize_t len = 0;
  chunk_table* tmptable = NULL;

  file = fopen(chunk_file, r);

  if(!flag)
    {
      getline(&buf, &n, file); // Remove "File: ..."
      getline(&buf, &n, file); // Remove "Chunks: ..."
      bzero(buf, n);
    }

  while((len = getline(&buf, &n, file)) != -1)
    {
      char tmpbuf[30] = {0};
      tmptable = calloc(1,sizeof(chunk_table));
      sscanf(buf, "%zu %s", &tmptable->id, &tmpbuf);
      ascii2hex(tmpbuf, HASH_SIZE+10, tmptable->chunk);
      HASH_ADD_KEYPTR(hh, *table, tmptable->chunk, HASH_SIZE, tmptable);
    }

  return;
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

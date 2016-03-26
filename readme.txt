@file   readme.txt
@author Fadhil Abubaker, Malek Anabtawi


                                                                                                                                                                                        
                                                                                                                                                                                        
BBBBBBBBBBBBBBBBB     iiii          tttt      TTTTTTTTTTTTTTTTTTTTTTT                                                                                                     tttt          
B::::::::::::::::B   i::::i      ttt:::t      T:::::::::::::::::::::T                                                                                                  ttt:::t          
B::::::BBBBBB:::::B   iiii       t:::::t      T:::::::::::::::::::::T                                                                                                  t:::::t          
BB:::::B     B:::::B             t:::::t      T:::::TT:::::::TT:::::T                                                                                                  t:::::t          
  B::::B     B:::::Biiiiiiittttttt:::::tttttttTTTTTT  T:::::T  TTTTTTooooooooooo   rrrrr   rrrrrrrrr   rrrrr   rrrrrrrrr       eeeeeeeeeeee    nnnn  nnnnnnnn    ttttttt:::::ttttttt    
  B::::B     B:::::Bi:::::it:::::::::::::::::t        T:::::T      oo:::::::::::oo r::::rrr:::::::::r  r::::rrr:::::::::r    ee::::::::::::ee  n:::nn::::::::nn  t:::::::::::::::::t    
  B::::BBBBBB:::::B  i::::it:::::::::::::::::t        T:::::T     o:::::::::::::::or:::::::::::::::::r r:::::::::::::::::r  e::::::eeeee:::::een::::::::::::::nn t:::::::::::::::::t    
  B:::::::::::::BB   i::::itttttt:::::::tttttt        T:::::T     o:::::ooooo:::::orr::::::rrrrr::::::rrr::::::rrrrr::::::re::::::e     e:::::enn:::::::::::::::ntttttt:::::::tttttt    
  B::::BBBBBB:::::B  i::::i      t:::::t              T:::::T     o::::o     o::::o r:::::r     r:::::r r:::::r     r:::::re:::::::eeeee::::::e  n:::::nnnn:::::n      t:::::t          
  B::::B     B:::::B i::::i      t:::::t              T:::::T     o::::o     o::::o r:::::r     rrrrrrr r:::::r     rrrrrrre:::::::::::::::::e   n::::n    n::::n      t:::::t          
  B::::B     B:::::B i::::i      t:::::t              T:::::T     o::::o     o::::o r:::::r             r:::::r            e::::::eeeeeeeeeee    n::::n    n::::n      t:::::t          
  B::::B     B:::::B i::::i      t:::::t    tttttt    T:::::T     o::::o     o::::o r:::::r             r:::::r            e:::::::e             n::::n    n::::n      t:::::t    tttttt
BB:::::BBBBBB::::::Bi::::::i     t::::::tttt:::::t  TT:::::::TT   o:::::ooooo:::::o r:::::r             r:::::r            e::::::::e            n::::n    n::::n      t::::::tttt:::::t
B:::::::::::::::::B i::::::i     tt::::::::::::::t  T:::::::::T   o:::::::::::::::o r:::::r             r:::::r             e::::::::eeeeeeee    n::::n    n::::n      tt::::::::::::::t
B::::::::::::::::B  i::::::i       tt:::::::::::tt  T:::::::::T    oo:::::::::::oo  r:::::r             r:::::r              ee:::::::::::::e    n::::n    n::::n        tt:::::::::::tt
BBBBBBBBBBBBBBBBB   iiiiiiii         ttttttttttt    TTTTTTTTTTT      ooooooooooo    rrrrrrr             rrrrrrr                eeeeeeeeeeeeee    nnnnnn    nnnnnn          ttttttttttt  
                                                                                                                                                                                        
                                                                                                                                                                                        
                                                                                                                                                                                        
                                                                                                                                                                                        
                                                                                                                                                                                        
                                                                                                                                                                                        
                                                                                                                                                                                        
peer.c implements a BitTorrent-like file transfer protocol that runs on top of
UDP. In order to achieve confident transfer of data, TCP-like reliability and
congesion control are implemented using UDP as the base.

Most of the implementation relies heavily on Hash Tables for storage and
retrieval of chunks; this was possible courtesy of 'uthash.h', an external hash
table library. For instance, the .hashchunks, .getchunks and .masterchunks were
all converted into Hash Tables for fast lookup and insertion. The complete list
of peers was also converted into a hash table, with "IPaddr:port" as the key.

Upon receiving a GET request from the user, the main routine selects peers
according to the chunks they have. Multiple peers are 'tagged' for retrieval,
and a queue of packets to be sent is maintained for each 'tagged' peer. This
queue is manipulated using the sliding window protocol. Akin to a coarse form of
multiplexing, the main routine loops through each peer and sends packets
destined to them based on a window.

Each peer also maintains a timestamp for timeout purposes. Everytime a packet is
received from that peer, the timestamp is updated. A stale timestamp is detected
by comparing a peer's timestamp with twice it's round-trip-time. This indicates
a timeout, and congestion-control protocols kick in to reduce window size and
reduce network congestion. The rtt is calculated using the Karn-Partridge
algorithm with alpha = 0.8.

Sample Output of problem2-peer.txt:

F1	2375	6
F1	2375	7
F1	2405	8
F1	2467	9
F1	2498	10
F1	2498	11
F1	2526	12
F1	2557	13
F1	2558	14
F1	2618	15
F1	2620	16
F1	2741	17
F1	2772	18

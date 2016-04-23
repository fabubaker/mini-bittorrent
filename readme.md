mini-p2p
========

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

# Authors 
[Fadhil Abubaker](https://github.com/fabubaker)
[Malek Anabtawi](https://github.com/js2d)

# Sample Output of problem2-peer.txt:

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

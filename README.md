# mini-p2p

Some time ago, I took [15-441: Computer Networks](https://web2.qatar.cmu.edu/~kharras/courses/15441/#/) and had a lot of
fun doing it. This repository contains code for project 2 of that course, titled "Congestion Control with Bittorrent".

# Building

Run

```
make
```

to build the `peer` binary, which is a client that implements our custom Bittorrent protocol.

# Running

NOTE: The custom bittorent protocol requires a bit of pre-processing of files before they can be used. The files in this
 example have already undergone pre-processing. To convert your own files for use with `peer`, follow the instructions
 [here](https://web2.qatar.cmu.edu/~kharras/courses/15441/projects/project2/project2.pdf#section.5). You might also have
 to download some of the starter code mentioned in there.

The files in the `example` directory are what we'll be using to demo the bittorrent peer. Before the demo, let's go
through the files that we'll be using.

Suppose `C.tar` is the file we're uploading to our bittorrent network.

The protocol works by having each peer own different chunks of `C.tar`, and have them communicate to get each other's
chunks. A chunk is 512KB and is identified by applying a cryptographic hash on its contents. In this case, `C.tar` has
4 chunks, and their hashes can be viewed in `C.masterchunks`.

`C.tar` is split into two equal sizes files `A.tar` and `B.tar`, having 2 chunks each. The hashes of their chunks can
be viewed in `A.haschunks` and `B.haschunks`.

Each peer's IP and port is described in `nodes.map`

We can now run the peers. Open a terminal, navigate to the root of the project and run:

```
./peer -p example/nodes.map -c example/A.haschunks -f example/C.masterchunks -m 4 -i 1
```

Here's an explanation of the options:

`-p <peer-list-file>` : Contains a list of all peers in the network with their IPs and ports.

`-f <master-chunk-file>`: The file containing a masterlist of all the chunks that are stored in the network.

`-c <has-chunk-file>` : The file that lists the chunks that will be owned by this peer. This is a subset of
`master-chunk-file`.

`-m <max-downloads>` : The maximum number of simultaneous connections allowed in each direction (download /upload).

`-i <peer-identity>` : The identity of the current peer.  This is used by the peer to get its hostname and port
from `peer-list-file`.

Now we run the second peer in another terminal:

```
./peer -p example/nodes.map -c example/B.haschunks -f example/C.masterchunks -m 4 -i 2
```

So in a nutshell, we have 2 peers running, the first one owns `A.haschunks` and the second owns `B.haschunks`.

To start the torrenting process, navigate to the terminal running the second peer, and type in:

```
GET example/A.chunks example/newA.tar
```

`A.chunks` is a file that contains the chunks that we want to retrieve from the first peer. Running the above
command starts a request to the first peer to download `A.chunks` from it and store it as `example/newA.tar`. Once the
download process is complete, the peer will print `GOT example/A.chunks`.

We can then verify that the newly obtained `newA.tar` from the first peer is the same as the original file that it was
derived from:

```
diff example/A.tar example/newA.tar
```

There should be no differences.

The process can then be repeated on the first peer to get `B.chunks` from the second peer by running:

```
GET example/B.chunks example/newB.tar
diff example/B.tar example/newB.tar
```

That's it for the demo!


# Submission Description

peer.c implements a BitTorrent-like file transfer protocol that runs on top of
UDP. In order to achieve confident transfer of data, TCP-like reliability and
congesion control are implemented using UDP as the base.

Most of the implementation relies heavily on Hash Tables for storage and
retrieval of chunks; this was possible courtesy of 'uthash.h', an external hash
table library. For instance, the .haschunks, .getchunks and .masterchunks were
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

[Malek Anabtawi](https://github.com/jspro123)

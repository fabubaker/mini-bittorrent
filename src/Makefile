# Some variables
CC 		= gcc
CFLAGS		= -g3 -std=gnu99 -Wall -pg
LDFLAGS		= -lm -lrt
TESTDEFS	= -DTESTING			# comment this out to disable debugging code
OBJS		= peer.o bt_parse.o debug.o input_buffer.o chunk.o sha.o spiffy.o
SRCOBJS = packet.o ds.o
VPATH 	= src

BINS            = peer make-chunks
TESTBINS        = test_debug test_input_buffer

# Implicit .o target
.c.o:
	$(CC) -c $(CFLAGS) $<

# Explicit build and testing targets

all: ${BINS}

bt_parse.c: bt_parse.h

clean:
	rm -f *.o peer

peer: $(OBJS) $(SRCOBJS)
	$(CC) $(CFLAGS) $(OBJS) $(SRCOBJS) -o $@ $(LDFLAGS)

make-chunks: $(MK_CHUNK_OBJS)
	$(CC) $(CFLAGS) $(MK_CHUNK_OBJS) -c -o $@ $(LDFLAGS)

# Debugging utility code

test:
	./peer -p ./example/test0/nodes.map -c ./example/test0/A.haschunks -f ./example/test0/C.chunks -m 4 -i 1 -d 6

test2:
	./peer -p ./example/test0/nodes.map -c ./example/test0/B.haschunks -f ./example/test0/C.chunks -m 4 -i 2 -d 6

test3:
	./peer -p ./example/test0/nodes.map -c ./example/test0/B.haschunks -f ./example/test0/C.chunks -m 4 -i 3 -d 6

debug-text.h: debug.h
	./debugparse.pl < debug.h > debug-text.h

test_debug.o: debug.c debug-text.h
	${CC} debug.c ${INCLUDES} ${CFLAGS} -c -D_TEST_DEBUG_ -o $@

test_input_buffer:  test_input_buffer.o input_buffer.o

test_packet.o: packet.c packet.h ds.o
			$(CC) $(CFLAGS) $(TESTDEFS) ds.o packet.c -o $@

test_ds.o: ds.c ds.h
		$(CC) $(CFLAGS) $(TESTDEFS) ds.c -o $@

handin:
	(make clean; tar cvf ../fabubake.tar ../15-441-project-2 --exclude example --exclude topo.map --exclude nodes.map --exclude hupsim.pl)

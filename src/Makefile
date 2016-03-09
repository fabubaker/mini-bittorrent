# Some variables
CC 		= gcc
CFLAGS		= -g3 -Wall -Wextra -Werror -DDEBUG
LDFLAGS		= -lm
TESTDEFS	= -DTESTING			# comment this out to disable debugging code
OBJS		= peer.o bt_parse.o debug.o input_buffer.o chunk.o sha.o
SRCOBJS = packet.o ds.o

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
	./peer -p ./example/nodes.map -c ./example/A.haschunks -f ./example/C.chunks -m 4 -i 1 -d 6

test2:
	./peer -p ./example/nodes.map -c ./example/B.haschunks -f ./example/C.chunks -m 4 -i 2 -d 6

debug-text.h: debug.h
	./debugparse.pl < debug.h > debug-text.h

test_debug.o: debug.c debug-text.h
	${CC} debug.c ${INCLUDES} ${CFLAGS} -c -D_TEST_DEBUG_ -o $@

test_input_buffer:  test_input_buffer.o input_buffer.o

test_packet.o: packet.c packet.h ds.o
			$(CC) $(CFLAGS) $(TESTDEFS) ds.o packet.c -o $@

test_ds.o: ds.c ds.h
		$(CC) $(CFLAGS) $(TESTDEFS) ds.c -o $@

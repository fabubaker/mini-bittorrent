# Some variables
CC 		= gcc
CFLAGS		= -g -Wall -Wextra -Werror -DDEBUG
LDFLAGS		= -lm
TESTDEFS	= -DTESTING			# comment this out to disable debugging code
OBJS		= peer.o bt_parse.o spiffy.o debug.o input_buffer.o chunk.o sha.o

packet.o: packet.c packet.h ds.o
		$(CC) $(CFLAGS) packet.c -o $@

ds.o: ds.c ds.h
	$(CC) $(CFLAGS) ds.c -c -o $@

test_ds.o: ds.c ds.h
		$(CC) $(CFLAGS) $(TESTDEFS) ds.c -c -o $@

clean:
	rm -f *.o

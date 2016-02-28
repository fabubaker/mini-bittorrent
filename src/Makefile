# Some variables
CC 		= gcc
CFLAGS		= -g -Wall -Wextra -Werror -DDEBUG
LDFLAGS		= -lm
TESTDEFS	= -DTESTING			# comment this out to disable debugging code
OBJS		= peer.o bt_parse.o spiffy.o debug.o input_buffer.o chunk.o sha.o

ds.o: ds.c ds.h
	$(CC) $(CFLAGS) ds.c -o $@

test_ds.o: ds.c ds.h
		$(CC) $(CFLAGS) $(TESTDEFS) ds.c -o $@

clean:
	rm -f *.o

CFLAGS=-std=c11 -g -static -fno-common
SRCS=$(wildcard *.c)
OBJS=$(SRCS:.c=.o)

litecc: $(OBJS)
	$(CC) -o $@ $? $(LDFLAGS)

$(OBJS): litecc.h

test: litecc
	./test.sh

clean:
	rm -f litecc *.o *~ tmp*

.PHONY: test clean
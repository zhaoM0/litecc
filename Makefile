CFLAGS=-std=c11 -g -static

nacc: main.o
	$(CC) -o $@ $? $(LDFLAGS)

test: nacc
	./test.sh

clean:
	rm -f nacc *.o *~ tmp*

.PHONY: test clean
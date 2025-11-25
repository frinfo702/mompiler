CFLAGS=-std=c11 -g
LDFLAGS=-static
SRCS=$(wildcard *.c)
OBJS=$(SRCS:.c=.o)

9cc: $(OBJS)
	$(CC) -o 9cc $(OBJS) $(LDFLAGS)

$(OBJS): 9cc.h

test: 9cc
	./test.sh

clean:
	rm -f 9cc *.o *~ tmp* tmp.s

.PHONY: test clean

compose/build:
	docker build --platform=linux/amd64 -t 9cc .

compose/up: compose/build
	docker run --platform=linux/amd64 -it --rm -v $(CURDIR):/home/user/work 9cc

CFLAGS ?= -std=c11 -g
LDFLAGS ?=
SRCS=$(wildcard *.c)
OBJS=$(addprefix objdir/,$(SRCS:.c=.o))
TARGET=./bin/9cc

all: $(TARGET)

$(TARGET): $(OBJS)
	mkdir -p ./bin
	$(CC) -o $(TARGET) $(OBJS) $(LDFLAGS)

objdir/%.o: %.c 9cc.h
	mkdir -p objdir
	$(CC) $(CFLAGS) -c $< -o $@

test: $(TARGET)
	./test.sh

clean:
	rm -rf ./bin ./tmpdir objdir
	

.PHONY: test clean all

compose/build:
	docker build --platform=linux/amd64 -t 9cc .

compose/up: compose/build
	docker run --platform=linux/amd64 -it --rm -v $(CURDIR):/home/user/work 9cc

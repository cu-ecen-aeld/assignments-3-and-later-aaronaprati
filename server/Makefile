CROSS_COMPILE ?=
#CROSS_COMPILE=aarch64-none-linux-gnu-
CC ?= gcc
OBJFILES ?= aesdsocket.o
CFLAGS ?= -g -Wall -Werror
LDFLAGS ?= -pthread

default: all

all: aesdsocket

aesdsocket: aesdsocket.o
	@if [ -z "$(CROSS_COMPILE)" ]; \
	then \
		$(CC) -o aesdsocket aesdsocket.o $(CFLAGS) $(LDFLAGS) -lpthread; \
	else \
		"$(CROSS_COMPILE)$(CC)" -o aesdsocket aesdsocket.o $(CFLAGS) $(LDFLAGS) -lpthread; \
	fi

aesdsocket.o: aesdsocket.c
	@if [ -z "$(CROSS_COMPILE)" ]; \
	then \
		$(CC) -c aesdsocket.c -o aesdsocket.o; \
	else \
		"$(CROSS_COMPILE)$(CC)" -c aesdsocket.c -o aesdsocket.o; \
	fi

clean:
	@rm -f aesdsocket aesdsocket.o


CC = gcc
CFLAGS = -g -Wall -Wextra -DDEBUG
LIBS = 

all: sockets

ifeq ($(OS), Windows_NT)
LIBS += -luser32 -lws2_32
CFLAGS += -DWIN_PLATFORM
else ifeq ($(shell "uname -s"), Linux)
CFLAGS += -DLINUX_PLATFORM
else ifeq ($(shell "uname -s"), Darwin)
CFLAGS += -DLINUX_PLATFORM
endif

sockets: sockets.c debug.c debug.h
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

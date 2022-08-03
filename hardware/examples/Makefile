ROOTDIR=..
LIBNAME=hardware

RM = rm
CC = gcc
LIBDIR = $(ROOTDIR)/lib
INCDIR = $(ROOTDIR)/include
CFLAGS = -Wall -Wextra -I$(INCDIR) -g
LDLIBS = -L$(LIBDIR) -l$(LIBNAME)

all: tmr

tmr.o: tmr.c
	$(CC) $(CFLAGS) -c tmr.c

tmr: tmr.o
	$(CC) -o tmr tmr.o $(LDLIBS)

clean:
	$(RM) -f *.o tmr vdisk?.bin

.PHONY: clean

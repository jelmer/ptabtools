prefix = /usr/local
bindir = $(prefix)/bin
mandir = $(prefix)/share/man
libdir = $(prefix)/lib
includedir = $(prefix)/include
PROGS = ptb2ly libptb-0.1.so
INSTALL = install
CFLAGS = -g -Wall

PTB2LY_OBJS = ptb2ly.o ptb.o
PTBSO_OBJS = ptb.o

all: $(PROGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< `pkg-config --cflags glib-2.0`

libptb-0.1.so: $(PTBSO_OBJS)
	$(CC) -shared $(CFLAGS) -o $@ $(PTBSO_OBJS) `pkg-config --libs glib-2.0`
	
ptb2ly: $(PTB2LY_OBJS)
	$(CC) $(CFLAGS) -o $@ $(PTB2LY_OBJS) `pkg-config --libs glib-2.0` -lpopt

install: all
	$(INSTALL) ptb2ly $(DESTDIR)$(bindir)
	$(INSTALL) ptb2ly.1 $(DESTDIR)$(mandir)/man1
	$(INSTALL) libptb-0.1.so $(DESTDIR)$(libdir)
	$(INSTALL) ptb.h $(DESTDIR)$(includedir)

tags:
	ctags *.c *.h

clean: 
	rm -f *.o core $(PROGS)

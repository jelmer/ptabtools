prefix = /usr/local
bindir = $(prefix)/bin
mandir = $(prefix)/share/man
libdir = $(prefix)/lib
includedir = $(prefix)/include
PTB_VERSION=0.1
PROGS = ptb2ly libptb-$(PTB_VERSION).so ptb2ascii
INSTALL = install
CFLAGS = -g -Wall -DPTB_VERSION=\"$(PTB_VERSION)\" 


PTB2LY_OBJS = ptb2ly.o ptb.o
PTB2ASCII_OBJS = ptb2ascii.o ptb.o
PTBSO_OBJS = ptb.o

all: $(PROGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< `pkg-config --cflags glib-2.0`

libptb-$(PTB_VERSION).so: $(PTBSO_OBJS)
	$(CC) -shared $(CFLAGS) -o $@ $(PTBSO_OBJS) `pkg-config --libs glib-2.0`
	
ptb2ascii: $(PTB2ASCII_OBJS)
	$(CC) $(CFLAGS) -o $@ $(PTB2ASCII_OBJS) `pkg-config --libs glib-2.0` -lpopt

ptb2ly: $(PTB2LY_OBJS)
	$(CC) $(CFLAGS) -o $@ $(PTB2LY_OBJS) `pkg-config --libs glib-2.0` -lpopt

install: all
	$(INSTALL) ptb2ly $(DESTDIR)$(bindir)
	$(INSTALL) ptb2ascii $(DESTDIR)$(bindir)
	$(INSTALL) -m 644 ptb2ly.1 $(DESTDIR)$(mandir)/man1
	$(INSTALL) -m 644 ptb2ascii.1 $(DESTDIR)$(mandir)/man1
	$(INSTALL) libptb-$(PTB_VERSION).so $(DESTDIR)$(libdir)
	$(INSTALL) -m 644 ptb.h $(DESTDIR)$(includedir)

tags:
	ctags *.c *.h

clean: 
	rm -f *.o core $(PROGS)

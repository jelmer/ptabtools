prefix = /usr/local
bindir = $(prefix)/bin
mandir = $(prefix)/share/man
libdir = $(prefix)/lib
includedir = $(prefix)/include
pkgconfigdir = $(libdir)/pkgconfig
PTB_VERSION=0.2
PROGS = ptb2ly ptb2ascii ptbinfo $(shell pkg-config --exists libxml-2.0 libxslt && echo ptb2xml)
LIBS = libptb-$(PTB_VERSION).so 
PROGS_MANPAGES = $(patsubst %,%.1,$(PROGS))
INSTALL = install
CFLAGS = -g -Wall -DPTB_VERSION=\"$(PTB_VERSION)\" 

PTB2LY_OBJS = ptb2ly.o ptb.o
PTBINFO_OBJS = ptbinfo.o ptb.o
PTB2ASCII_OBJS = ptb2ascii.o ptb.o
PTB2XML_OBJS = ptb2xml.o ptb.o
PTBSO_OBJS = ptb.o

all: $(PROGS) $(LIBS)

ptb2xml.o: ptb2xml.c
	$(CC) $(CFLAGS) -c $< `pkg-config --cflags glib-2.0 libxml-2.0 libxslt`

%.o: %.c
	$(CC) $(CFLAGS) -c $< `pkg-config --cflags glib-2.0`

libptb-$(PTB_VERSION).so: $(PTBSO_OBJS)
	$(CC) -shared $(CFLAGS) -o $@ $(PTBSO_OBJS) `pkg-config --libs glib-2.0`

ptb2xml: $(PTB2XML_OBJS)
	$(CC) $(CFLAGS) -o $@ $(PTB2XML_OBJS) `pkg-config --libs glib-2.0 libxml-2.0 libxslt` -lpopt
	
ptb2ascii: $(PTB2ASCII_OBJS)
	$(CC) $(CFLAGS) -o $@ $(PTB2ASCII_OBJS) `pkg-config --libs glib-2.0` -lpopt

ptb2ly: $(PTB2LY_OBJS)
	$(CC) $(CFLAGS) -o $@ $(PTB2LY_OBJS) `pkg-config --libs glib-2.0` -lpopt

ptbinfo: $(PTBINFO_OBJS)
	$(CC) $(CFLAGS) -o $@ $(PTBINFO_OBJS) `pkg-config --libs glib-2.0` -lpopt

install: all
	$(INSTALL) $(PROGS) $(DESTDIR)$(bindir)
	$(INSTALL) -m 644 $(PROGS_MANPAGES) $(DESTDIR)$(mandir)/man1
	$(INSTALL) $(LIBS) $(DESTDIR)$(libdir)
	$(INSTALL) -m 644 ptb.h $(DESTDIR)$(includedir)
	$(INSTALL) -m 644 ptabtools.pc $(DESTDIR)$(pkgconfigdir)

tags: *.c *.h
	ctags *.c *.h

clean: 
	rm -f *.o core $(PROGS) $(LIBS)

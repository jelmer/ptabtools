prefix = /usr/local
MKDIR = mkdir
bindir = $(prefix)/bin
mandir = $(prefix)/share/man
libdir = $(prefix)/lib
includedir = $(prefix)/include
pkgconfigdir = $(libdir)/pkgconfig
datadir = $(prefix)/share/ptabtools
PTB_VERSION=0.4

PROGS = ptb2ly ptb2ascii ptbinfo $(shell pkg-config --exists libxml-2.0 libxslt && echo ptb2xml)
LIBS = libptb-$(PTB_VERSION).so 
PROGS_MANPAGES = $(patsubst %,%.1,$(PROGS))
INSTALL = install
CFLAGS = -g -Wall -DPTB_VERSION=\"$(PTB_VERSION)\" 
XSLT_DEFINE = $(shell pkg-config --exists libxslt && echo -DHAVE_XSLT) -DMUSICXMLSTYLESHEET=\"$(datadir)/ptbxml2musicxml.xsl\"

PTB2LY_OBJS = ptb2ly.o ptb.o
PTBINFO_OBJS = ptbinfo.o ptb.o
PTB2ASCII_OBJS = ptb2ascii.o ptb.o
PTB2XML_OBJS = ptb2xml.o ptb.o
PTBSO_OBJS = ptb.o

all: $(PROGS) $(LIBS)

ptb2xml.o: ptb2xml.c
	$(CC) $(CFLAGS) -c $< `pkg-config --cflags glib-2.0 libxml-2.0 libxslt` $(XSLT_DEFINE)

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
	$(MKDIR) -p $(DESTDIR)$(mandir)/man1
	$(INSTALL) -m 644 $(PROGS_MANPAGES) $(DESTDIR)$(mandir)/man1
	$(MKDIR) -p $(DESTDIR)$(libdir)
	$(INSTALL) $(LIBS) $(DESTDIR)$(libdir)
	$(MKDIR) -p $(DESTDIR)$(includedir)
	$(INSTALL) -m 644 ptb.h $(DESTDIR)$(includedir)
	$(MKDIR) -p $(DESTDIR)$(pkgconfigdir)
	$(INSTALL) -m 644 ptabtools.pc $(DESTDIR)$(pkgconfigdir)
	$(MKDIR) -p $(DESTDIR)$(datadir)
	$(INSTALL) -m 6444 ptbxml2musicxml.xsl $(DESTDIR)$(datadir)

ctags: tags
tags: *.c *.h
	ctags *.c *.h

clean: 
	rm -f *.o core $(PROGS) $(LIBS)

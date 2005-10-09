-include Makefile.settings

PTBLIB_OBJS = ptb.o gp.o ptb-tuning.o
TARGETS = $(TARGET_BINS) $(TARGET_LIBS)

all: $(TARGETS)

ptb2xml.o: ptb2xml.c
	$(CC) $(CFLAGS) -c $< $(LIBXSLT_CFLAGS) $(LIBXML_CFLAGS) $(XSLT_DEFINE)

%.o: %.c
	$(CC) $(CFLAGS) -c $< 

%.po: %.c
	$(CC) $(CFLAGS) -fPIC -c $< -o $@

ptb.dll: $(PTBLIB_OBJS)
	$(CC) $(SHFLAGS) $(CFLAGS) -Wl,--out-implib=ptb.dll.a -o $@ $^

libptb.so.$(VERSION): $(PTBLIB_OBJS:.o=.po)
	$(CC) -Wl,-soname,libptb.so.0 $(SHFLAGS) $(CFLAGS) -o $@ $^

libptb.a: $(PTBLIB_OBJS)
	$(AR) rs $@ $^

ptb2xml$(EXEEXT): ptb2xml.o ptb.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS) $(LIBXML_LIBS) $(LIBXSLT_LIBS) $(POPT_LIBS)
	
ptb2ascii$(EXEEXT): ptb2ascii.o ptb.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS) $(POPT_LIBS)

ptb2ptb$(EXEEXT): ptb2ptb.o ptb.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS) $(POPT_LIBS)

ptb2ly$(EXEEXT): ptb2ly.o ptb.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS) $(POPT_LIBS)

ptb2abc$(EXEEXT): ptb2abc.o ptb.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS) $(POPT_LIBS)

gp2ly$(EXEEXT): gp2ly.o gp.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS) $(POPT_LIBS)

ptbinfo$(EXEEXT): ptbinfo.o ptb.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS) $(POPT_LIBS)

ptbdict$(EXEEXT): ptbdict.o ptb.o ptb-tuning.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS) $(POPT_LIBS)
	
install: all
	$(INSTALL) -d $(DESTDIR)$(bindir)
	test -z "$(TARGET_BINS)" || $(INSTALL) $(TARGET_BINS) $(DESTDIR)$(bindir)
	$(INSTALL) -d $(DESTDIR)$(mandir)/man1
	$(INSTALL) -m 644 $(PROGS_MANPAGES) $(DESTDIR)$(mandir)/man1
	$(INSTALL) -d $(DESTDIR)$(libdir)
	test -z "$(TARGET_LIBS)" || $(INSTALL) -m 644 $(TARGET_LIBS) $(DESTDIR)$(libdir)
	$(INSTALL) -d $(DESTDIR)$(includedir)
	$(INSTALL) -m 644 ptb.h $(DESTDIR)$(includedir)
	$(INSTALL) -m 644 gp.h $(DESTDIR)$(includedir)
	$(INSTALL) -d $(DESTDIR)$(pkgconfigdir)
	$(INSTALL) -m 644 ptabtools.pc $(DESTDIR)$(pkgconfigdir)
	$(INSTALL) -d $(DESTDIR)$(datadir)
	$(INSTALL) -m 644 ptbxml2musicxml.xsl $(DESTDIR)$(datadir)
	$(INSTALL) -m 644 ptbxml.dtd $(DESTDIR)$(datadir)

test: all ptb2ptb
	$(MAKE) -C tests

cags: tags
tags: $(wildcard *.c) $(wildcard *.h)
	ctags *.c *.h

clean: 
	rm -f *.o core $(TARGETS)

realclean: clean
	rm -f Makefile.settings configure

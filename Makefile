PROGS = ptb2ly ptbsplit ptbtest

PTB2LY_OBJS = ptb2ly.o ptb.o sections.o
PTBSPLIT_OBJS = ptb.o ptbsplit.o
PTBTEST_OBJS = ptb.o ptbtest.o sections.o


all: $(PROGS)

%.o: %.c
	$(CC) -c $< `pkg-config --cflags glib-2.0`


ptbsplit: $(PTBSPLIT_OBJS)
	$(CC) -o $@ $(PTBSPLIT_OBJS) `pkg-config --libs glib-2.0`

ptb2ly: $(PTB2LY_OBJS)
	$(CC) -o $@ $(PTB2LY_OBJS) `pkg-config --libs glib-2.0`

ptbtest: $(PTBTEST_OBJS)
	$(CC) -o $@ $(PTBTEST_OBJS) `pkg-config --libs glib-2.0`

clean: 
	rm -f *,o core $(PROGS)

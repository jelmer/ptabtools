PROGS = ptb2ly ptbtest
CFLAGS = -g3

PTB2LY_OBJS = ptb2ly.o ptb.o sections.o
PTBSPLIT_OBJS = ptb.o ptbsplit.o
PTBTEST_OBJS = ptb.o ptbtest.o sections.o


all: $(PROGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< `pkg-config --cflags glib-2.0`


ptbsplit: $(PTBSPLIT_OBJS)
	$(CC) $(CFLAGS) -o $@ $(PTBSPLIT_OBJS) `pkg-config --libs glib-2.0`

ptb2ly: $(PTB2LY_OBJS)
	$(CC) $(CFLAGS) -o $@ $(PTB2LY_OBJS) `pkg-config --libs glib-2.0`

ptbtest: $(PTBTEST_OBJS)
	$(CC) $(CFLAGS) -o $@ $(PTBTEST_OBJS) `pkg-config --libs glib-2.0`

clean: 
	rm -f *.o core $(PROGS)

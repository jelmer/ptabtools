PROGS = ptb2ly
CFLAGS = -g -Wall

PTB2LY_OBJS = ptb2ly.o ptb.o sections.o

all: $(PROGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< `pkg-config --cflags glib-2.0`


ptb2ly: $(PTB2LY_OBJS)
	$(CC) $(CFLAGS) -o $@ $(PTB2LY_OBJS) `pkg-config --libs glib-2.0` -lpopt

tags:
	ctags *.c *.h

clean: 
	rm -f *.o core $(PROGS)

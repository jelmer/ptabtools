all: ptb2ly

%.o: %.c
	$(CC) -c $< `pkg-config --cflags glib-2.0`

ptb2ly: ptb2ly.o ptb.o
	$(CC) -o $@ ptb2ly.o ptb.o `pkg-config --libs glib-2.0`

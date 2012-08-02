
OBJS = game.o

LIBS = -lncurses 
CFLAGS = -Wall -g -O2

snake: $(OBJS)
	cc $(LIBS) $< -o $@

clean:
	rm -rf *.o snake

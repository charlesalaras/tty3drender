CC=gcc
CFLAGS=-lm -DTB_OPT_ATTR_W=64

compile:
	$(CC)  main.c $(CFLAGS)

clean:
	rm a.out

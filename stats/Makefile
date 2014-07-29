CC=gcc
CFLAGS=-ggdb3
INCLUDE=-I../include
all : stats 

stats : stats.o
	$(CC) $(CFLAGS) stats.o -o stats

stats.o : stats.c ../include/stats.h
	$(CC) $(CFLAGS) -o stats.o -c stats.c $(INCLUDE)
clean :
	rm *.o stats

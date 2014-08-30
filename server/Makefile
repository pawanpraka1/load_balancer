CC=gcc
CFLAGS=-ggdb3
INCLUDE=-I../include

all : server

server : server.o backend_server.o event_handler.o stats_handler.o session.o
	$(CC) $(CFLAGS) server.o backend_server.o event_handler.o stats_handler.o session.o -o server

server.o : server.c ../include/server.h ../include/stats.h
	$(CC) $(CFLAGS) -c server.c -o server.o $(INCLUDE)

backend_server.o : backend_server.c ../include/server.h ../include/stats.h
	$(CC) $(CFLAGS) -c backend_server.c -o backend_server.o $(INCLUDE)

event_handler.o : event_handler.c ../include/server.h ../include/stats.h
	$(CC) $(CFLAGS) -c event_handler.c -o event_handler.o $(INCLUDE)

stats_handler.o : stats_handler.c ../include/server.h ../include/stats.h
	 $(CC) $(CFLAGS) -c stats_handler.c -o stats_handler.o $(INCLUDE)

session.o : session.c ../include/server.h
	$(CC) $(CFLAGS) -c session.c -o session.o $(INCLUDE)
clean :
	rm -f *.o server

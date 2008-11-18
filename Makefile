SERVERFLAGS = -DSERVER_BUILD -D_REENTRANT -o server -I. -lpthread
CLIENTFLAGS = -DCLIENT_BUILD -o client -I.
ALLFLAGS = -DSERVER_BUILD -D_REENTRANT -o server -I. -lpthread
CDEBUG = -g -Wall
server: server.c tftp.h
	gcc $(CDEBUG) server.c tftp.h $(SERVERFLAGS)
client: client.c tftp.h
	gcc $(CDEBUG) client.c tftp.h $(CLIENTFLAGS)
all: server.c tftp.h client.c
	gcc $(CDEBUG) server.c tftp.h $(SERVERFLAGS)
	gcc $(CDEBUG) client.c tftp.h $(CLIENTFLAGS)
clean:
	rm client server
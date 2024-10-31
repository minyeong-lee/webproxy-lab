# Makefile for Proxy Lab 
#
# You may modify this file any way you like (except for the handin
# rule). You instructor will type "make" on your specific Makefile to
# build your proxy from sources.

CC = gcc
CFLAGS = -g -Wall
LDFLAGS = -lpthread

all: proxy

csapp.o: csapp.c csapp.h
	$(CC) $(CFLAGS) -c csapp.c

proxy.o: proxy.c csapp.h
	$(CC) $(CFLAGS) -c proxy.c

proxy: proxy.o csapp.o
	$(CC) $(CFLAGS) proxy.o csapp.o -o proxy $(LDFLAGS)

# Creates a tarball in ../proxylab-handin.tar that you can then
# hand in. DO NOT MODIFY THIS!
handin:
	(make clean; cd ..; tar cvf $(USER)-proxylab-handin.tar proxylab-handout --exclude tiny --exclude nop-server.py --exclude proxy --exclude driver.sh --exclude port-for-user.pl --exclude free-port.sh --exclude ".*")

clean:
	rm -f *~ *.o proxy core *.tar *.zip *.gzip *.bzip *.gz

# echo 추가
echoclient.o: echo-client.c csapp.h
	$(CC) $(CFLAGS) -c echo-client.c

echoclient: echo-client.o csapp.o
	$(CC) $(CFLAGS) echo-client.o csapp.o -o echoclient $(LDFLAGS)

echoserver.o: echo-server.c csapp.h
	$(CC) $(CFLAGS) -c echo-server.c

echoserver: echo-server.o csapp.o
	$(CC) $(CFLAGS) echo-server.o csapp.o -o echoserver $(LDFLAGS)



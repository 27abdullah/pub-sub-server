CC = gcc
CFLAGS = -pedantic -Wall -std=gnu99 -I/local/courses/csse2310/include -pthread
LDFLAGS = -L/local/courses/csse2310/lib -lcsse2310a3 -lstringmap -lcsse2310a4

LIBCFLAGS=-fPIC -Wall -pedantic -std=gnu99 -L/local/courses/csse2310/lib -lstringmap -I/local/courses/csse2310/include/

PROG_S = psserver
SOURCE_S = server.c clientList.c
PROG_C = psclient
SOURCE_C = client.c

all: ps
ps: psserver psclient stringmap.o libstringmap.so

psserver: $(SOURCE_S)
	$(CC) $(CFLAGS) $(LDFLAGS) $(SOURCE_S) -o $(PROG_S)

psclient: $(SOURCE_C)
	$(CC) $(CFLAGS) $(LDFLAGS) $(SOURCE_C) -o $(PROG_C)

clean_server:
	rm -f *.o psserver

clean_client:
	rm -f *.o psclient

# Turn stringmap.c into stringmap.o
stringmap.o: stringmap.c
	$(CC) $(LIBCFLAGS) -c $<

# Turn stringmap.o into shared library libstringmap.so
libstringmap.so: stringmap.o
	$(CC) -shared -o $@ stringmap.o




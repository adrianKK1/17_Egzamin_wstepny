CC = gcc
CFLAGS = -Wall -Wextra -pthread
# -Wall: wszystkie ostrzeżenia, -pthread: obsługa wątków

all: dziekan komisja kandydat

dziekan: dziekan.c utils.c common.h
	$(CC) $(CFLAGS) -o dziekan dziekan.c utils.c

komisja: komisja.c utils.c common.h
	$(CC) $(CFLAGS) -o komisja komisja.c utils.c

kandydat: kandydat.c utils.c common.h
	$(CC) $(CFLAGS) -o kandydat kandydat.c utils.c

clean:
	rm -f dziekan komisja kandydat *.o
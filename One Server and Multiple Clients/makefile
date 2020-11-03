all: sr ct

sr: server.c
	gcc -Wall -O1 -pthread server.c -o sr

ct: client.c
	gcc -Wall -O1 -pthread client.c -o ct

clean:
	rm -rf sr
	rm -rf ct
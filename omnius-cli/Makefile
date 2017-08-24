DEBUG="-g"

all: omnius-cli

omnius-cli: ../omnius/comm.o omnius-cli.o
	gcc $(DEBUG) -Wall -o $@ ../omnius/comm.o omnius-cli.o

omnius-cli.o: omnius-cli.c
	gcc $(DEBUG) -Wall -c -o $@ $<

../omnius/comm.o: ../omnius/comm.c
	gcc $(DEBUG) -Wall -c -o $@ $<

clean:
	rm -f omnius-cli.o ../omnius/comm.o omnius-cli

all: omnius-cli

omnius-cli: ../omnius/comm.o omnius-cli.o
	gcc -Wall -o $@ $<

omnius-cli.o: omnius-cli.c
	gcc -Wall -c -o $@ $<

../omnius/comm.o: ../omnius/comm.c
	gcc -Wall -c -o $@ $<

clean:
	rm -f *.o omnius-cli

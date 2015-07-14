all: omnius

omnius: regex_parse_dir
	gcc  -Wall -O -c ./*.c
	g++ -Wall -o omnius *.o regex_parse/*.o
regex_parse_dir:
	export DEBUG
	$(MAKE) -C regex_parse
clean: 
	rm -f omnius *.o 
	$(MAKE) clean -C regex_parse

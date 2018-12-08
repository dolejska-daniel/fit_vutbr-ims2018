# Makefile
# IMS, 07.12.2018
# Author: Daniel Dolejska, FIT

CC=g++
CFLAGS=-O3 -std=c++14 -Wall -Wextra
COUT=simulace

all:
#	if ldconfig -p | grep -q simlib; then
#		echo "SIMLIB is not installed. Cannot continue.";
#		exit 1;
#	fi;
	make simulace

run:
	make simulace
	./$(COUT)


simulace: main.o InputParser.o
	$(CC) $(CFLAGS) main.o InputParser.o -o $(COUT) -lsimlib

InputParser.o:
	$(CC) $(CFLAGS) src/ims2018/InputParser.cpp -c

main.o:
	$(CC) $(CFLAGS) src/ims2018/main.cpp -c


clean:
	rm *.o *.od *.tar.gz $(COUT) -f -v 2>/dev/null

pack:
	tar -czvf 2_xdolej08.tar.gz src/ims2018/* src/simlib/simlib.h doc/manual.pdf Makefile

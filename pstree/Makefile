CC = g++
CFLAGS = -Wall -Wextra -Werror
SRC := $(shell find . -maxdepth 1 -name "*.cpp")
DEPS := $(shell find . -maxdepth 1 -name "*.h") $(SRC)

pstree: pstree.o
	@g++ $(CFLAGS) $^ -o $@ 

pstree.o: pstree.cpp
	@g++ $(CFLAGS) -c $< 

clean:
	rm -f *.o pstree
		

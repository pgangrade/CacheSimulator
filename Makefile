CXX = g++
DEBUG_FLAGS = -O2 -g -Wall -Wextra -DDEBUG -std=gnu++11
RELEASE_FLAGS= -O3 -march=native -Wall -Wextra -std=gnu++11
CXXFLAGS=$(RELEASE_FLAGS)
#CXXFLAGS=$(DEBUG_FLAGS)
DEPS=$(wildcard *.h) Makefile
OBJ=system.o cache.o prefetch.o

all: cache

cache: main.cpp $(DEPS) $(OBJ)
	$(CXX) $(CXXFLAGS) -o cache main.cpp $(OBJ)

%.o: %.cpp $(DEPS)
	$(CXX) $(CXXFLAGS) -o $@ -c $<

.PHONY: clean
clean:
	rm -f *.o cache tags cscope.out

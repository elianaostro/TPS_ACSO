# Compiler settings
CXX = g++
CXXFLAGS = -std=c++11 -Wall -pthread -g

# Common source files
COMMON_SRC = thread-pool.cc Semaphore.cc

# Default target
all: main

# Main program
main: $(COMMON_SRC) main.cc
	$(CXX) $(CXXFLAGS) -o $@ $^

# Test programs
tptest: $(COMMON_SRC) tptest.cc
	$(CXX) $(CXXFLAGS) -o $@ $^

tpcustomtest: $(COMMON_SRC) tpcustomtest.cc
	$(CXX) $(CXXFLAGS) -o $@ $^

# Clean up
clean:
	rm -f main tptest tpcustomtest

.PHONY: all clean
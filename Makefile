CXX = g++
CXXFLAGS = -std=c++17 -Wall -Iinclude

COMPILER_SRC = $(wildcard src/compiler/*.cpp)
CIRCUIT_SRC = $(wildcard src/circuitBuilder/*.cpp)

all: compiler circuitBuilder

compiler:
	$(CXX) $(CXXFLAGS) $(COMPILER_SRC) -o compiler

circuitBuilder:
	$(CXX) $(CXXFLAGS) $(CIRCUIT_SRC) -o circuitBuilder

clean:
	rm -f compiler circuitBuilder
CXX = g++
CXXFLAGS = -std=c++17 -Wall -Iinclude -fopenmp

COMPILER_SRC = $(wildcard src/compiler/*.cpp)
CIRCUIT_SRC = \
	$(wildcard src/circuitBuilder/*.cpp) \
	$(wildcard src/circuitBuilder/core/*.cpp) \
	$(wildcard src/circuitBuilder/netlist/*.cpp) \
	$(wildcard src/circuitBuilder/utils/*.cpp) \
	$(wildcard src/simulator/*.cpp) \
	$(wildcard src/simulator/sequential/*.cpp) \
	$(wildcard src/simulator/parallel/*.cpp) 

all: compiler circuitBuilder

compiler:
	$(CXX) $(CXXFLAGS) $(COMPILER_SRC) -o compiler

circuitBuilder:
	$(CXX) $(CXXFLAGS) $(CIRCUIT_SRC) -o circuitBuilder

clean:
	rm -f compiler circuitBuilder
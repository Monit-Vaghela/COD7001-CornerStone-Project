CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2 -Iinclude

SRC_DIRS = src/core src/netlist src/utils
SRCS = src/main.cpp $(foreach dir,$(SRC_DIRS),$(wildcard $(dir)/*.cpp))

TARGET = simulator

all:
	$(CXX) $(CXXFLAGS) $(SRCS) -o $(TARGET)

clean:
	rm -f $(TARGET)

run: all
	./$(TARGET)

.PHONY: all clean run
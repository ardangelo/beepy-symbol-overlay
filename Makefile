CXX ?= g++
CXXFLAGS := -g -O2 -std=c++17 $(CXXFLAGS)

.PHONY: clean

all: symbol-overlay

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

symbol-overlay: src/symbol-overlay.o
	$(CXX) $< -o $@

clean:
	rm -f src/*.o symbol-overlay

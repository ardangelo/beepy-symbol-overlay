CXX ?= g++
CXXFLAGS := -g -O2 -std=c++17 $(CXXFLAGS)

.PHONY: clean

all: symbol-overlay

%.psf: %.psf.gz
	gunzip -k $^

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $^ -o $@

symbol-overlay: src/main.o src/overlay.o src/psf.o
	$(CXX) $^ -o $@

clean:
	rm -f src/*.o symbol-overlay

CXX ?= g++
CXXFLAGS := -O2 -std=c++17 $(CXXFLAGS)

.PHONY: clean

all: symbol-overlay

%.psf: %.psf.gz
	gunzip -k $^

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $^ -o $@

symbol-overlay: src/main.o src/KeymapRender.o src/Overlay.o src/PSF.o src/x11name_to_utf16.o
	$(CXX) $^ -o $@

clean:
	rm -f src/*.o symbol-overlay

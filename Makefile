CXX ?= g++
OBJCOPY ?= objcopy
CXXFLAGS := -g -O2 -std=c++17 $(CXXFLAGS)

.PHONY: clean

all: symbol-overlay

%.psf: %.psf.gz
	gunzip -k $^

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $^ -o $@

# Debug info hangs compiler
x11name_to_utf16.o: x11name_to_utf16.cpp
	$(CXX) -O2 -std=c++17 $(CXXFLAGS) -c $^ -o $@

src/font.o: font.psf
	$(OBJCOPY) -O elf32-littlearm -I binary $< $@

symbol-overlay: src/main.o src/KeymapRender.o src/Overlay.o src/PSF.o src/x11name_to_utf16.o src/font.o
	$(CXX) -static $^ -o $@

clean:
	rm -f src/*.o symbol-overlay

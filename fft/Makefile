# Host-side FFT model and hardware comparison.
#
#   make          build gen_vectors and compare
#   make vectors  regenerate stimulus.hex and expected.txt from the C++ model
#   make check    regenerate the vectors, then compare against ip_output.txt
#   make clean    remove the built programs

CXXFLAGS ?= -std=c++17 -O2 -Wall -Wextra -Wpedantic

PROGS := gen_vectors compare

all: $(PROGS)

gen_vectors: gen_vectors.cpp fft.hpp signalGen.hpp
	$(CXX) $(CXXFLAGS) -o $@ $<

compare: compare_fft.cpp
	$(CXX) $(CXXFLAGS) -o $@ $<

vectors: gen_vectors
	./gen_vectors

check: vectors compare
	./compare

clean:
	rm -f $(PROGS)

.PHONY: all vectors check clean

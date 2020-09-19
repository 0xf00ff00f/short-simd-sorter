CXXFLAGS=-O3 -std=c++17

%.o: %.c $(DEPS)
	$(CXX) -c -o $@ $< $(CXXFLAGS)

all: test

gen-sorter: gen-sorter.o
	$(CXX) -o $@ $^ $(CXXFLAGS)

sort.cc: gen-sorter
	./gen-sorter > sort.cc

test: test.o sort.o
	$(CXX) -o $@ $^ $(CXXFLAGS)

clean:
	rm -f *.o test gen-sorter sort.cc

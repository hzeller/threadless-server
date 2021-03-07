CXXFLAGS=-std=c++11 -Wall -W

all : threadless-server fortune-teller

threadless-server: threadless-server.o fd-mux.o
	$(CXX) $^ -o $@

fortune-teller: fortune-teller.o fd-mux.o
	$(CXX) $^ -o $@

clean:
	rm -f *.o threadless-server fortune-teller

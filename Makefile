CXXFLAGS=-std=c++11 -Wall -W

threadless-server: threadless-server.o fd-mux.o
	$(CXX) $^ -o $@

clean:
	rm -f *.o threadless-server

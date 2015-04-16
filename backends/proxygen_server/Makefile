CFLAGS=-std=c++11
LDLIBS=-lcryptopp -lproxygenhttpserver -lfolly -lglog -lgflags -pthread

main:
	g++ -o server EchoServer.cpp EchoHandler.cpp Storage.cpp $(CFLAGS) $(LDLIBS) 

clean:
	rm *.o
	rm server

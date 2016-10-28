# makefile

all: dataserver client 
semaphore.o: semaphore.h semaphore.cpp
	g++ -std=c++11 -c -g semaphore.cpp

#bounded_buffer.o: bounded_buffer.h
#	g++ -std=c++11 -c -g bounded_buffer.h

reqchannel.o: reqchannel.h reqchannel.cpp
	g++ -std=c++11 -c -g reqchannel.cpp

dataserver: dataserver.cpp reqchannel.o 
	g++ -std=c++11 -g -o dataserver dataserver.cpp reqchannel.o -lpthread

client: client_MP6.cpp reqchannel.o semaphore.o
	g++ -std=c++11 -g -o client client_MP6.cpp bounded_buffer.h reqchannel.o semaphore.o -lpthread

clean:
	rm *.o fifo* dataserver client output*

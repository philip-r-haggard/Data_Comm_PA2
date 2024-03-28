all: client server

client: client.o packet.o
	g++ client.o packet.o -o client
	
server: server.o packet.o
	g++ server.o packet.o -o server	

packet: packet.o
	g++ packet.o -o packet
	
client.o: client.cpp packet.h
	g++ -c client.cpp

server.o: server.cpp packet.h
	g++ -c server.cpp

packet.o: packet.cpp packet.h
	g++ -c packet.cpp

clean:
	rm -f *.o client server packet
	rm -f output.txt
	rm -f clientseqnum.log
	rm -f clientack.log
	rm -f arrival.log
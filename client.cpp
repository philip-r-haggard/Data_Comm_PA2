/*

Authors: Philip Haggard (prh148) & Lael Lum (all655)

Sources consulted:

digitalocean.com
w3schools.com
geeksforgeeks.com
stackoverflow.com

*/

#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <iostream>
#include <fstream>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <cstring>
#include <stdlib.h>
#include <cstdlib>
#include "packet.h"
#include <math.h>
#include <time.h>

#define CHUNK_SIZE 5

using namespace std;

int main(int argc, char *argv[]) {

	// resolve the hostname to an IP address
	int packetLen = 50;
	struct hostent *s;
	s = gethostbyname(argv[1]);

	// declare and initialize socket variables
	struct sockaddr_in server;
	int mysocket = 0;
	socklen_t slen = sizeof(server);

	// create a new socket
	if ((mysocket=socket(AF_INET, SOCK_DGRAM, 0))==-1)
		cout << "Error in creating socket.\n";

	// configure the server address
	memset((char *) &server, 0, sizeof(server));
	server.sin_family = AF_INET;
	uint16_t port = atoi(argv[2]); // convert the negotiated port number from the command line to an integer
	server.sin_port = htons(port); // and use it in the new socket
	bcopy((char *)s->h_addr,
		(char *)&server.sin_addr.s_addr,
		s->h_length);

    // read data from file and send data in chunks of 4 bytes to the server through the new socket
	ifstream file(argv[3], ios_base::in | ios_base::binary);

    if (!file) {
		cout << "Error opening file.\n";
	} else {
		char payload[512];
		char spacket[packetLen];
		memset(spacket, 0, packetLen);
		int sequenceNumber = 0;

		while (!file.eof())
		{
			file.read(payload, CHUNK_SIZE); // read the data from the file in chunks of 5 characters

			payload[file.gcount()] = '\0'; // mark the end of the chunk will a null character
			int bytesRead = file.gcount();

			packet mySendPacket(1, sequenceNumber, bytesRead, payload);

			if(sequenceNumber == 0)
			{
				sequenceNumber = 1;
			}
			else
			{
				sequenceNumber = 0;
			}

			mySendPacket.serialize(spacket);

			if (sendto(mysocket, spacket, 50, 0, (struct sockaddr *)&server, slen) == -1) {
				cout << "Error in file sendto function.\n";
				goto jmp;
			}
		}

		packet mySendPacket(2, sequenceNumber, 0, nullptr);
		mySendPacket.serialize(spacket);
		sendto(mysocket, spacket, 50, 0, (struct sockaddr *)&server, slen);
	}

	jmp:

	// close the file
	file.close();

	// close socket
	close(mysocket);

	return 0;
}
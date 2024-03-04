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
#include <time.h>
#include <string.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <unistd.h>
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <cstring>
#include <stdlib.h>
#include <netdb.h>
#include "packet.h"

#define CHUNK_SIZE 4

using namespace std;

int main(int argc, char *argv[]) {

	// declare and initialize socket variables
	struct sockaddr_in server;
	struct sockaddr_in client;
	int mysocket = 0;
	int i = 0;
	socklen_t clen = sizeof(client);

	char payload[512];
	memset(payload, 0, 512);
	char serialized[512];
	memset(serialized, 0, 512);

	packet receivedPacket(0, 0, 0, payload);

	// create a new socket
	if ((mysocket=socket(AF_INET, SOCK_DGRAM, 0))==-1)
		cout << "Error in socket creation.\n";

	// configure server address
	memset((char *) &server, 0, sizeof(server));
	server.sin_family = AF_INET;
	uint16_t port = atoi(argv[1]);
	server.sin_port = htons(port);
	server.sin_addr.s_addr = htonl(INADDR_ANY);
	if (bind(mysocket, (struct sockaddr *)&server, sizeof(server)) == -1)
		cout << "Error in binding.\n";

    // initialize client structure
	memset((char *)&client, 0, sizeof(client));
	client.sin_family = AF_INET;
	client.sin_port = htons(port);
	client.sin_addr.s_addr = htonl(INADDR_ANY);

	// receive data in chunks of 4 bytes from client through socket and write to upload.txt
	ofstream file("output.txt", ios::binary);

	char *dataPointer;
	char *outputData;

	if (!file.is_open()) {
		cout << "Error opening file for writing.\n";
	} else {
		while ((recvfrom(mysocket, serialized, 512, 0, (struct sockaddr *)&client, &clen)) > 0) {
			receivedPacket.deserialize(serialized);
			receivedPacket.printContents();

			dataPointer = receivedPacket.getData();

			file << dataPointer;

			if (receivedPacket.getType() == 2)
			{
				goto jmp;
			}
		}
	}

	jmp:

	// close the file
	file.close();

	// close the socket
	close(mysocket);

	return 0;
}
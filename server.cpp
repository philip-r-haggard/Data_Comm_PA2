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

	if (!file.is_open()) {
		cout << "Error opening file for writing.\n";
	} else {
		char chunk[CHUNK_SIZE + 1];		    // initializing the size of each data chunk holder
		char response[CHUNK_SIZE + 1];	    // with (+ 1) to account for a null terminator
		ssize_t bytes_received;
        const char* myString = "!";
        strcpy(response, myString);

		while ((bytes_received = recvfrom(mysocket, chunk, CHUNK_SIZE, 0, (struct sockaddr *)&client, &clen)) > 0) {
			if (bytes_received == 1 && chunk[0] == '\n') {
				goto jmp;
			}

			file.write(chunk, bytes_received);

            response[bytes_received] = '\0';

			if (sendto(mysocket, response, sizeof(response), 0, (struct sockaddr *)&client, clen)==-1) {
				cout << "Error in sendto function.\n";
			}

			if (bytes_received == -1) {
				cout << "Error receiving data.\n";
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
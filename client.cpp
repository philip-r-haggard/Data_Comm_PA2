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

#define CHUNK_SIZE 4

using namespace std;

int main(int argc, char *argv[]) {

	// resolve the hostname to an IP address
	struct hostent *s;
	s = gethostbyname(argv[1]);

	// declare and initialize socket variables
	struct sockaddr_in server;
	int mysocket = 0;
	socklen_t slen = sizeof(server);
	char handshake[512] = "1248";

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

	// send handshake to server through socket
	if (sendto(mysocket, handshake, 8, 0, (struct sockaddr *)&server, slen)==-1)
		cout << "Error in sendto function.\n";

	// receive random port number from server through socket
	recvfrom(mysocket, handshake, 512, 0, (struct sockaddr *)&server, &slen);

	// close negotiated port socket
	close(mysocket);

	// declare and initialize new socket variables
	struct sockaddr_in new_server;
	int new_socket = 0;
	socklen_t new_slen = sizeof(new_server);

	// create a new socket
	if ((new_socket=socket(AF_INET, SOCK_DGRAM, 0))==-1)
		cout << "Error in creating socket.\n";

	// configure the server address
	memset((char *) &new_server, 0, sizeof(new_server));
	new_server.sin_family = AF_INET;
	uint16_t new_port = atoi(handshake);	// convert the random port number from the sever to an integer
	new_server.sin_port = htons(new_port);	// and use it in the new socket
	bcopy((char *)s->h_addr,
        	(char *)&new_server.sin_addr.s_addr,
        	s->h_length);

	// read data from file and send data in chunks of 4 bytes to the server through the new socket
	ifstream file(argv[3], ios_base::in | ios_base::binary);

	if (!file) {
		cout << "Error opening file.\n";
	} else {
		char chunk[CHUNK_SIZE + 1];		// initializing the size
		char response[CHUNK_SIZE + 1];		// of each data chunk holder with (+ 1) to
		char allCapitals[CHUNK_SIZE + 1];	// account for a null terminator
		ssize_t bytes_received;

		while (file.read(chunk, CHUNK_SIZE)) {
			chunk[file.gcount()] = '\0';
			if (sendto(new_socket, chunk, file.gcount(), 0, (struct sockaddr *)&new_server, new_slen) == -1) {
				cout << "Error in file sendto function.\n";
                goto jmp;
			}

			bytes_received = recvfrom(new_socket, response, CHUNK_SIZE, 0, (struct sockaddr *)&new_server, &new_slen);

			for (int x = 0; x < bytes_received; x++) {
				allCapitals[x] = toupper(chunk[x]);
			}

			allCapitals[bytes_received] = '\0';
			response[bytes_received] = '\0';

			if ((strcmp(allCapitals, response)) != 0) {
				cout << "Error in file transfer process.\n";
				goto jmp;
			}
			
			cout << response << endl;

		}

		const char delimiter[] = "\n";
		if (sendto(new_socket, delimiter, strlen(delimiter), 0, (struct sockaddr *)&new_server, new_slen) == -1) {
			cout << "Error sending delimiter.\n";
		}
	}

	jmp:

	// close the file
	file.close();

	// close random port socket
	close(new_socket);

	return 0;
}
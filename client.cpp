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

    // READ FROM FILE AND SEND TO SERVER HERE

    // read data from file and send data in chunks of 4 bytes to the server through the new socket
	ifstream file(argv[3], ios_base::in | ios_base::binary);

    if (!file) {
		cout << "Error opening file.\n";
	} else {
		char chunk[CHUNK_SIZE + 1];		    // initializing the size
		char response[CHUNK_SIZE + 1];		// of each data chunk holder with (+ 1) to
		char allCapitals[CHUNK_SIZE + 1];	// account for a null terminator
		ssize_t bytes_received;

		while (file.read(chunk, CHUNK_SIZE)) {
			chunk[file.gcount()] = '\0';
			if (sendto(mysocket, chunk, file.gcount(), 0, (struct sockaddr *)&server, slen) == -1) {
				cout << "Error in file sendto function.\n";
                goto jmp;
			}

			bytes_received = recvfrom(mysocket, response, CHUNK_SIZE, 0, (struct sockaddr *)&server, &slen);
			
			cout << response << endl;

		}
	}

	jmp:

	// close the file
	file.close();

	// close socket
	close(mysocket);

	return 0;
}
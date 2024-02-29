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

#define CHUNK_SIZE 4

using namespace std;

int main(int argc, char *argv[]) {

	// declare and initialize socket variables
	struct sockaddr_in server;
	struct sockaddr_in client;
	int mysocket = 0;
	int i = 0;
	socklen_t clen = sizeof(client);
	char handshake[512];

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

	// receive handshake from client through socket
	if (recvfrom(mysocket, handshake, 512, 0, (struct sockaddr *)&client, &clen)==-1)
		cout << "Failed to receive.\n";

	// generate random port number
	srand(time(nullptr));
	int randomPort = rand() % (65535 - 1024 + 1) + 1024;
	string randomPortStr = to_string(randomPort);
	cout << "My random port is: " << randomPort << endl;
	const char* response = randomPortStr.c_str();

	// send random port number to client through socket
	if (sendto(mysocket, response, sizeof(response), 0, (struct sockaddr *)&client, clen)==-1){
		cout << "Error in sendto function.\n";
	}

	// close negotiated port socket
	close(mysocket);

	// declare and initialize new socket variables
	struct sockaddr_in new_server;
	struct sockaddr_in new_client;
	int new_socket = 0;
	socklen_t new_clen = sizeof(new_client);

	// create a new socket
	if ((new_socket=socket(AF_INET, SOCK_DGRAM, 0))==-1)
		cout << "Error in socket creation.\n";

	// configure the server address
	memset((char *) &new_server, 0, sizeof(new_server));
	new_server.sin_family = AF_INET;
	uint16_t new_port = randomPort;
	new_server.sin_port = htons(new_port);
	new_server.sin_addr.s_addr = htonl(INADDR_ANY);
	if (bind(new_socket, (struct sockaddr *)&new_server, sizeof(new_server)) == -1)
		cout << "Error in binding.\n";

	// initialize new_client structure
	memset((char *)&new_client, 0, sizeof(new_client));
	new_client.sin_family = AF_INET;
	new_client.sin_port = htons(new_port);
	new_client.sin_addr.s_addr = htonl(INADDR_ANY);

	// receive data in chunks of 4 bytes from client through socket and write to upload.txt
	ofstream file("upload.txt", ios::binary);

	if (!file.is_open()) {
		cout << "Error opening file for writing.\n";
	} else {
		char chunk[CHUNK_SIZE + 1];		// initializing the size of each data chunk holder
		char allCapitals[CHUNK_SIZE + 1];	// with (+ 1) to account for a null terminator
		ssize_t bytes_received;

		while ((bytes_received = recvfrom(new_socket, chunk, CHUNK_SIZE, 0, (struct sockaddr *)&new_client, &new_clen)) > 0) {
			if (bytes_received == 1 && chunk[0] == '\n') {
				goto jmp;
			}

			file.write(chunk, bytes_received);

			for (int x = 0; x < bytes_received; x++) {
				allCapitals[x] = toupper(chunk[x]);
			}

			allCapitals[bytes_received] = '\0';

			if (sendto(new_socket, allCapitals, sizeof(allCapitals), 0, (struct sockaddr *)&new_client, new_clen)==-1) {
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
	close(new_socket);

	return 0;
}
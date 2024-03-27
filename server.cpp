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

#define packetLen 50

using namespace std;

int main(int argc, char *argv[]) {

	struct sockaddr_in server, client;
	int mysocket = 0;
	int i = 0;
	socklen_t clen = sizeof(client);
	char buf[packetLen];

	char payload[512];
	memset(payload, 0, 512);
	char serialized[512];
	memset(serialized, 0, 512);
	
	if ((mysocket=socket(AF_INET, SOCK_DGRAM, 0))==-1)
		cout << "Error in socket creation.\n";
	
	memset((char *) &server, 0, sizeof(server));
	server.sin_family = AF_INET;
	server.sin_port = htons(7123);
	server.sin_addr.s_addr = htonl(INADDR_ANY);
	if (bind(mysocket, (struct sockaddr *)&server, sizeof(server)) == -1) {
		cout << "Error in binding.\n";
		return 1;
	}

	packet rcvdPacket(0,0,0,payload);
	int bytes_received;
	while (true) {
        // Receive a packet
		bytes_received = 0;
        if ((bytes_received = recvfrom(mysocket, serialized, 512, 0, (struct sockaddr *) &client, (socklen_t *) &clen)) == -1) {
            cerr << "Error in receiving packet." << endl;
            break;
        }

		cout << "Received " << bytes_received << " bytes." << endl;

		rcvdPacket.deserialize(serialized);

		cout << "Past deserialization\n";

    	rcvdPacket.printContents();

		cout << "Past the deserialization" << endl;
        
		// If it's an end of transmission packet, break the loop
		if (rcvdPacket.getType() == 3)
			break;
    }
	
	close(mysocket);
	return 0;
}
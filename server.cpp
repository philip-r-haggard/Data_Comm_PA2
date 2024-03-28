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

	int expected_sequence_number = 0;

	while (true) {
        // Receive a packet
		bytes_received = 0;
        if ((bytes_received = recvfrom(mysocket, serialized, 512, 0, (struct sockaddr *) &client, (socklen_t *) &clen)) == -1) {
            cerr << "Error in receiving packet." << endl;
            break;
        }

		cout << "Received " << bytes_received << " bytes." << endl;

		rcvdPacket.deserialize(serialized);

		// Check if EOT packet received
		if (rcvdPacket.getType() == 3) {
			// Send ACK for EOT packet
			packet eotAckPacket(0, expected_sequence_number, 0, nullptr);
			char eotAckSerialized[packetLen];
			memset(eotAckSerialized, 0, packetLen);
			eotAckPacket.serialize(eotAckSerialized);
			sendto(mysocket, eotAckSerialized, packetLen, 0, (struct sockaddr *) &client, clen);
			cout << "Received EOT. Sending ACK for EOT." << endl;
			goto end; // Exit loop after receiving EOT
		}

		// Check if received packet has the expected sequence number
        if (rcvdPacket.getSeqNum() == expected_sequence_number) {
            // Send ACK for the received packet
            packet ackPacket(0, expected_sequence_number, 0, nullptr);
            char ackSerialized[packetLen];
            memset(ackSerialized, 0, packetLen);
            ackPacket.serialize(ackSerialized);
            sendto(mysocket, ackSerialized, packetLen, 0, (struct sockaddr *) &client, clen);

            // Print received payload
            cout << "Received: " << rcvdPacket.getData() << endl;

            // Increment expected sequence number
            expected_sequence_number = (expected_sequence_number + 1) % 2;
        } else {
            // Resend ACK for the last successfully received packet
            packet ackPacket(0, (expected_sequence_number + 1) % 2, 0, nullptr);
            char ackSerialized[packetLen];
            memset(ackSerialized, 0, packetLen);
            ackPacket.serialize(ackSerialized);
            sendto(mysocket, ackSerialized, packetLen, 0, (struct sockaddr *) &client, clen);
            cout << "Discarded out-of-order packet. Resending ACK for last successfully received packet." << endl;
        }
    }

	end:
	
	close(mysocket);
	return 0;
}
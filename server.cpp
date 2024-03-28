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

void writeToLogFile(const string& filename, int seqNum) {
    ofstream file(filename, ios::out | ios::app);  // Open file for writing (overwrite if exists)
    if (file.is_open()) {
        file << seqNum << "\n";  // Write sequence number to file
        file.close();  // Close the file
    } else {
        std::cerr << "Error: Unable to open file " << filename << " for writing." << endl;
    }
}

int main(int argc, char *argv[]) {

	// declare and setup server
	struct hostent *em_host;            // pointer to a structure of type hostent
	em_host = gethostbyname(argv[1]);   // host name for emulator
	
	if(em_host == NULL){                // failed to obtain server's name
		cout << "Failed to obtain server.\n";
		exit(EXIT_FAILURE);
	}

	// ******************************************************************
	// ******************************************************************
	
	// sets up datagram socket for receiving from emulator
	int ESSocket = socket(AF_INET, SOCK_DGRAM, 0);  
	if(ESSocket < 0){
		cout << "Error: failed to open datagram socket.\n";
	}

	// set up the sockaddr_in structure for receiving
	struct sockaddr_in ES; 
	socklen_t ES_length = sizeof(ES);
	bzero(&ES, sizeof(ES)); 
	ES.sin_family = AF_INET;	
	ES.sin_addr.s_addr = htonl(INADDR_ANY);	
	char * end;
	int sr_rec_port = strtol(argv[2], &end, 10);  // server's receiving port and convert to int
	ES.sin_port = htons(sr_rec_port);             // set to emulator's receiving port

	// do the binding
	if (bind(ESSocket, (struct sockaddr *)&ES, ES_length) == -1)
		cout << "Error in binding.\n";		
		
	// ******************************************************************
	// ******************************************************************

	int SESocket = socket(AF_INET, SOCK_DGRAM, 0);
	if(SESocket < 0){
		cout << "Error in trying to open datagram socket.\n";
		exit(EXIT_FAILURE);
	}
		
	// setup sockaddr_in structure for sending
	struct sockaddr_in SE;	
	memset((char *) &SE, 0, sizeof(SE));
	SE.sin_family = AF_INET;
	bcopy((char *)em_host->h_addr, (char*)&SE.sin_addr.s_addr, em_host->h_length);
	int em_rec_port = strtol(argv[3], &end, 10);
	SE.sin_port = htons(em_rec_port);

	// ******************************************************************
	// ******************************************************************

	char payload[512];
	memset(payload, 0, 512);
	char serialized[512];
	memset(serialized, 0, 512);

	packet rcvdPacket(0,0,0,payload);
	int bytes_received;

	int expected_sequence_number = 0;
	ofstream outputFile(argv[4]); // Open the output file for writing

	while (true) {
        // Receive a packet
		bytes_received = 0;
		char payload[512];
		memset(payload, 0, 512);
		char serialized[512];
		memset(serialized, 0, 512);

		packet rcvdPacket(0,0,0,payload);
        if ((bytes_received = recvfrom(ESSocket, serialized, 512, 0, (struct sockaddr *) &ES, (socklen_t *) &ES_length)) == -1) {
            cerr << "Error in receiving packet." << endl;
            goto end;
        }

		rcvdPacket.deserialize(serialized);

		writeToLogFile("arrival.log", rcvdPacket.getSeqNum());

		// Check if EOT packet received
		if (rcvdPacket.getType() == 3) {
			// Send ACK for EOT packet
			packet eotAckPacket(2, expected_sequence_number, 0, nullptr);
			char eotAckSerialized[packetLen];
			memset(eotAckSerialized, 0, packetLen);
			eotAckPacket.serialize(eotAckSerialized);
			sendto(SESocket, eotAckSerialized, packetLen, 0, (struct sockaddr *) &SE, sizeof(struct sockaddr_in));
			goto end;
		}

		// Check if received packet has the expected sequence number
        if (rcvdPacket.getSeqNum() == expected_sequence_number) {
            // Send ACK for the received packet
            packet ackPacket(0, expected_sequence_number, 0, nullptr);
            char ackSerialized[packetLen];
            memset(ackSerialized, 0, packetLen);
            ackPacket.serialize(ackSerialized);
            sendto(SESocket, ackSerialized, packetLen, 0, (struct sockaddr *) &SE, sizeof(struct sockaddr_in));

			// Get the data from the received packet
			char *data = rcvdPacket.getData();

			// Write the data to the output file
			if (data != nullptr) {
				int dataLength = strlen(data);
				for (int j = 0; data[j] != '\0'; j++) {
					if (data[j] == '\n') {
						outputFile << endl; // Start a new line if newline character is encountered
					} else {
						outputFile << data[j]; // Write the character to the file
					}
				}
			} else {
				cout << "No data received in packet." << endl;
			}

            // Increment expected sequence number
            expected_sequence_number = (expected_sequence_number + 1) % 2;
        } else {
            // Resend ACK for the last successfully received packet
            packet ackPacket(0, (expected_sequence_number + 1) % 2, 0, nullptr);
            char ackSerialized[packetLen];
            memset(ackSerialized, 0, packetLen);
            ackPacket.serialize(ackSerialized);
            sendto(SESocket, ackSerialized, packetLen, 0, (struct sockaddr *) &SE, sizeof(struct sockaddr_in));
            cout << "Discarded out-of-order packet. Resending ACK for last successfully received packet." << endl;
        }
    }

	end:
	
	outputFile.close();
	close(ESSocket);
	close(SESocket);
	return 0;
}
// Authors: Philip Haggard (prh148) and Lael Lum (all655)

// To test this code, you need two shells open on Pluto
// run ./server
// run ./client localhost

//#include <stdlib.h>
#include <cstring>
#include <cstdlib>
#include <sys/types.h>   // defines types (like size_t)
#include <sys/socket.h>  // defines socket class
#include <netinet/in.h>  // defines port numbers for (internet) sockets, some address structures, and constants
#include <netdb.h> 
#include <iostream>
#include <fstream>
#include <arpa/inet.h>   // if you want to use inet_addr() function
#include <string.h>
#include <unistd.h>
#include "packet.h" // include packet class
#include <math.h>
#include <time.h>

using namespace std;

#define payLen 30
#define packetLen 50
#define TIMEOUT 2 // Timeout value in seconds

// Function to write sequence number to log file
void writeToLogFile(const string& filename, int seqNum) {
    ofstream file(filename, ios::out | ios::app);  // Open file for writing (overwrite if exists)
    if (file.is_open()) {
        file << seqNum << "\n";  // Write sequence number to file
        file.close();  // Close the file
    } else {
        std::cerr << "Error: Unable to open file " << filename << " for writing." << endl;
    }
}

int main(int argc, char *argv[]){

    // declare and setup server
	struct hostent *em_host;            // pointer to a structure of type hostent
	em_host = gethostbyname(argv[1]);   // host name for emulator
	if(em_host == NULL){                // failed to obtain server's name
		cout << "Failed to obtain server.\n";
		exit(EXIT_FAILURE);
	}

    // ******************************************************************
    // ******************************************************************

    // client sets up datagram socket for sending
    int CESocket = socket(AF_INET, SOCK_DGRAM, 0);  
    if(CESocket < 0){
        cout << "Error: failed to open datagram socket.\n";
    }

    // set up the sockaddr_in structure for sending
    struct sockaddr_in CE; 
    socklen_t CE_length = sizeof(CE);
    bzero(&CE, sizeof(CE)); 
    CE.sin_family = AF_INET;	
    bcopy((char *)em_host->h_addr, (char*)&CE.sin_addr.s_addr, em_host->h_length);  // both using localhost so this is fine
    char * end;
    int em_rec_port = strtol(argv[2], &end, 10);  // get emulator's receiving port and convert to int
    CE.sin_port = htons(em_rec_port);             // set to emulator's receiving port

    // ******************************************************************
    // ******************************************************************

    // client sets up datagram socket for receiving
    int ECSocket = socket(AF_INET, SOCK_DGRAM, 0);  
    if(ECSocket < 0){
        cout << "Error: failed to open datagram socket.\n";
    }

    // set up the sockaddr_in structure for receiving
    struct sockaddr_in EC;
    socklen_t EC_length = sizeof(EC);
    bzero(&EC, sizeof(EC)); 
    EC.sin_family = AF_INET;	
    EC.sin_addr.s_addr = htonl(INADDR_ANY);	
    char * end2;
    int cl_rec_port = strtol(argv[3], &end2, 10);  // client's receiving port and convert to int
    EC.sin_port = htons(cl_rec_port);             // set to emulator's receiving port

    // do the binding
    if (bind(ECSocket, (struct sockaddr *)&EC, EC_length) == -1){
            cout << "Error in binding.\n";
    } 
    
    // ******************************************************************
    // ******************************************************************

    // Set timeout for the socket
    struct timeval timeout;
    timeout.tv_sec = 2; // Timeout in seconds
    timeout.tv_usec = 0;
    if (setsockopt(ECSocket, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout)) < 0) {
        cerr << "Error setting socket timeout." << endl;
        return 1;
    }

    // ******************************************************************
    // ******************************************************************

    ifstream file(argv[4], ios_base::in | ios_base::binary);
    if (!file.is_open()) {
        cerr << "Failed to open file!\n";
        return 1;
    }

    char payload[512];
    memset(payload, 0, 512);
    char serialized[512];
    memset(serialized, 0, 512);

    int type;
    int sequence_number = 0; // Initialize sequence number

    packet rcvdAckPacket(0,0,0,payload);
    int bytes_received;

    // Initialize the timer variables
    time_t start_time;
    time_t current_time;
    double elapsed_time;
    
    while (file.good()) {
        memset(payload, 0, 512);
        file.read(payload, 5);
        int bytesRead = file.gcount(); // Get the actual number of bytes read
        if (bytesRead == 0) {
            goto end; // End of file reached
        }

        //payload[bytesRead] = '\0'; // Null-terminate the buffer
        for (int i = bytesRead; i < sizeof(payload); ++i) {
            payload[i] = '\0';
        }

        char spacket[packetLen]; // Add space for null termination
        memset(spacket, 0, packetLen);

        packet mySendPacket(1, sequence_number, strlen(payload), payload); // make the packet to be serialized and sent

        // Serialize packet
        mySendPacket.serialize(spacket);

        // Send packet
        sendto(CESocket, spacket, packetLen, 0, (struct sockaddr *) &CE, sizeof(CE));

        //Log sent sequence number
        writeToLogFile("clientseqnum.log", sequence_number);

        time_t start = time(NULL);

        while (true) {
            // Receive ACK
            bytes_received = 0;
            if ((bytes_received = recvfrom(ECSocket, serialized, 512, 0, (struct sockaddr *) &EC, &EC_length)) == -1) {
                if (errno == EWOULDBLOCK || errno == EAGAIN) { // Timeout occurred
                    cerr << "Timeout occurred. Retransmitting packet..." << endl;
                    // Resend packet
                    sendto(CESocket, spacket, packetLen, 0, (struct sockaddr *) &CE, sizeof(CE));
                    // Restart timer
                    start = time(NULL);
                } else {
                    cerr << "Error in receiving ACK." << endl;
                    return 1;
                }
            } else {
                // Deserialize ACK
                rcvdAckPacket.deserialize(serialized);
                // Check if ACK is for the expected packet
                if (rcvdAckPacket.getSeqNum() == 0 || rcvdAckPacket.getSeqNum() == 1) {
                    writeToLogFile("clientack.log", rcvdAckPacket.getSeqNum());
                    break;
                }
            }

            // Check if timeout has occurred
            if (difftime(time(NULL), start) >= 2) {
                cerr << "Timeout occurred. Retransmitting packet..." << endl;
                // Resend packet
                sendto(CESocket, spacket, packetLen, 0, (struct sockaddr *) &CE, sizeof(CE));
                // Restart timer
                start = time(NULL);
            }
        }

        sequence_number = (sequence_number + 1) % 2;

        if (bytesRead < 5)
        {
            goto end;
        }
    }

    end:

    // Send the EOT packet
    packet clientEOT(3, sequence_number, 0, nullptr);
    char eot[packetLen];
    memset(eot, 0, packetLen);
    clientEOT.serialize(eot);
    sendto(CESocket, eot, packetLen, 0, (struct sockaddr *) &CE, sizeof(CE));

    //Log sent sequence number
    writeToLogFile("clientseqnum.log", sequence_number);

    // Receive the EOT packet from the server
    if ((bytes_received = recvfrom(ECSocket, serialized, 512, 0, (struct sockaddr *) &EC, &EC_length)) == -1) {
        cout << "Error occurred in receiving EOT packet\n";
    }

    //Log received sequence number
    rcvdAckPacket.deserialize(serialized);
    writeToLogFile("clientack.log", rcvdAckPacket.getSeqNum());

    file.close();
    close(CESocket);
    close(ECSocket);

    return 0;
}
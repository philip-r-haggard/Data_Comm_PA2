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

#define packetLen 50
#define TIMEOUT 2 // Timeout value in seconds

int main(int argc, char *argv[]){
    
    if (argc != 3) {
        cerr << "Usage: " << argv[0] << " <server_hostname> <file_path>" << endl;
        return 1;
    }

    ifstream file(argv[2], ios_base::in | ios_base::binary);
    if (!file.is_open()) {
        cerr << "Failed to open file!\n";
        return 1;
    }

    char payload[512];
    memset(payload, 0, 512);
    char serialized[512];
    memset(serialized, 0, 512);

    struct hostent *s; 
    s = gethostbyname(argv[1]);

    struct sockaddr_in server;
    int mysocket = 0;
    socklen_t slen = sizeof(server); 

    if ((mysocket=socket(AF_INET, SOCK_DGRAM, 0))==-1)
        cout << "Error in creating socket.\n";
        
    memset((char *) &server, 0, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_port = htons(7123);
    bcopy((char *)s->h_addr, 
        (char *)&server.sin_addr.s_addr,
        s->h_length);

    // Set timeout for the socket
    struct timeval timeout;
    timeout.tv_sec = 2; // Timeout in seconds
    timeout.tv_usec = 0;
    if (setsockopt(mysocket, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout)) < 0) {
        cerr << "Error setting socket timeout." << endl;
        return 1;
    }

    int type;
    int sequence_number = 0; // Initialize sequence number

    packet rcvdAckPacket(0,0,0,payload);
    int bytes_received;

    // Initialize the timer variables
    time_t start_time;
    time_t current_time;
    double elapsed_time;
    
    while (!file.eof()) {
        file.read(payload, 5); // Read a chunk from the file
        int bytesRead = file.gcount(); // Get the actual number of bytes read
        if (bytesRead == 0) {
            break; // End of file reached
        }

        payload[bytesRead] = '\0'; // Null-terminate the buffer

        // Print the null-terminated string
        cout << "[" << payload << "]" << endl;

        // Create packet
        cout << "Bytes read: " << bytesRead << endl;

        char spacket[packetLen]; // Add space for null termination
        memset(spacket, 0, packetLen);

        packet mySendPacket(1, sequence_number, strlen(payload), payload); // make the packet to be serialized and sent

        // Serialize packet
        mySendPacket.serialize(spacket);

        // Send packet
        sendto(mysocket, spacket, packetLen, 0, (struct sockaddr *) &server, sizeof(server));

        time_t start = time(NULL);

        while (true) {
            // Receive ACK
            bytes_received = 0;
            if ((bytes_received = recvfrom(mysocket, serialized, 512, 0, (struct sockaddr *) &server, &slen)) == -1) {
                if (errno == EWOULDBLOCK || errno == EAGAIN) { // Timeout occurred
                    cerr << "Timeout occurred. Retransmitting packet..." << endl;
                    // Resend packet
                    sendto(mysocket, spacket, packetLen, 0, (struct sockaddr *) &server, slen);
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
                    // ACK received, break out of loop
                    cout << "Received ACK for sequence number " << rcvdAckPacket.getSeqNum() << endl;
                    break;
                }
            }

            // Check if timeout has occurred
            if (difftime(time(NULL), start) >= 2) {
                cerr << "Timeout occurred. Retransmitting packet..." << endl;
                // Resend packet
                sendto(mysocket, spacket, packetLen, 0, (struct sockaddr *) &server, slen);
                // Restart timer
                start = time(NULL);
            }
        }

        sequence_number = (sequence_number + 1) % 2;
    }

    packet clientEOT(3, 0, 0, nullptr);
    char eot[packetLen];
    memset(eot, 0, packetLen);
    clientEOT.serialize(eot);
    sendto(mysocket, eot, packetLen, 0, (struct sockaddr *) &server, sizeof(server));
    cout << "Sending EOT packet!!!\n";
    goto end;

    end:

    file.close();
    close(mysocket);

    return 0;
}
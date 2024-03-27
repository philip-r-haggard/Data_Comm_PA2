// Authors: Philip Haggard (prh148) and Lael Lum (all655)

// To test this code, you need two shells open on Pluto
// run ./server
// run ./client localhost

#include <stdlib.h>
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

    char payload[512];
    int type;
    int sequence_number;
    
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

        packet mySendPacket(1, 0, strlen(payload), payload); // make the packet to be serialized and sent

        // Serialize packet
        mySendPacket.serialize(spacket);

        // Send packet
        sendto(mysocket, spacket, packetLen, 0, (struct sockaddr *) &server, sizeof(server));

        if (bytesRead < 5) {
            packet clientEOT(3, 0, 0, nullptr);
            char eot[bytesRead + 1];
            memset(eot, 0, bytesRead + 1);
            mySendPacket.serialize(eot);
            sendto(mysocket, eot, packetLen, 0, (struct sockaddr *) &server, sizeof(server));
            goto end;
        }
    }

    end:

    file.close();
    close(mysocket);

    return 0;
}
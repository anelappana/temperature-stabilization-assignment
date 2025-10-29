#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdbool.h>
#include "utils.h"

int main (int argc, char *argv[])
{
    int socket_desc;
    struct sockaddr_in server_addr;
    char server_message[100], client_message[100];

    struct msg the_message;
    
    // Command-line input arguments (user provided)
    if (argc < 3) {  // basic validation
        fprintf(stderr, "Usage: %s <external index 1..4> <initial temperature>\n", argv[0]);
        return 1;
    }
    int externalIndex = atoi(argv[1]);
    float initialTemperature = atof(argv[2]);

    // Create socket:
    socket_desc = socket(AF_INET, SOCK_STREAM, 0);
    if(socket_desc < 0){
        printf("Unable to create socket\n");
        return -1;
    }
    printf("Socket created successfully\n");

    // Set port and IP the same as server-side:
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(2000);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    // Send connection request to server:
    if(connect(socket_desc, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0){
        printf("Unable to connect\n");
        return -1;
    }
    printf("Connected with server successfully\n");
    printf("--------------------------------------------------------\n\n");
       
    // Package to be sent to server (send your initial temp once)
    the_message = prepare_message(externalIndex, initialTemperature, false);   // add 'false'
    if(send(socket_desc, (const void *)&the_message, sizeof(the_message), 0) < 0){
        printf("Unable to send message\n");
        return -1;
    }

    bool stable = false;     // proper loop control
    while (!stable)
    {
        // Receive the server's response:
        if(recv(socket_desc, (void *)&the_message, sizeof(the_message), 0) <= 0){
            printf("Error while receiving server's msg\n");
            return -1;
        }

        if (the_message.done) {          // stop signal from server
            printf("*** External(%d) stabilized. Final T=%f (central=%f) ***\n",
                   externalIndex, initialTemperature, the_message.T);
            break;
        }
        
        printf("--------------------------------------------------------\n");
        printf("Updated temperature sent by the Central process = %f\n", the_message.T);

        // Apply the assignment's external update rule:
        // externalTemp <- (3*externalTemp + 2*centralTemp)/5
        initialTemperature = (3.0f*initialTemperature + 2.0f*the_message.T) / 5.0f;

        // Send the updated external temp back to the server
        the_message = prepare_message(externalIndex, initialTemperature, false);
        if(send(socket_desc, (const void *)&the_message, sizeof(the_message), 0) < 0){
            printf("Unable to send message\n");
            return -1;
        }
    }

    // Close the socket:
    close(socket_desc);
    return 0;
}
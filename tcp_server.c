#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdbool.h>
#include <math.h>
#include "utils.h"

#define numExternals 4     // Number of external processes
#define EPS 1e-3           // convergence tolerance

int * establishConnectionsFromExternalProcesses()
{
    // This socket is used by the server (i.e., Central process) to listen for
    // connections from the External process. 
    int socket_desc;

    // There will be 4 client sockets, one for each external process.
    static int client_socket[numExternals];

    unsigned int client_size;               // (keep your type)
    struct sockaddr_in server_addr, client_addr;

    // Create socket:
    socket_desc = socket(AF_INET, SOCK_STREAM, 0);
    if(socket_desc < 0){
        printf("Error while creating socket\n");
        exit(0);
    }
    printf("Socket created successfully\n");

    // Optional but helpful
    int yes = 1;
    setsockopt(socket_desc, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));
    
    // Set port and IP:
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(2000);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    
    // Bind to the set port and IP:
    if(bind(socket_desc, (struct sockaddr*)&server_addr, sizeof(server_addr))<0){
        printf("Couldn't bind to the port\n");
        exit(0);
    }
    printf("Done with binding\n");
    
    // Listen for clients:
    if(listen(socket_desc, numExternals) < 0){      // allow 4 queued
        printf("Error while listening\n");
        exit(0);
    }
    printf("\n\nListening for incoming connections.....\n\n");

    printf("-------------------- Initial connections ---------------------------------\n");

    //========================================================
    //  Connections from externals 
    //========================================================
    int externalCount = 0; 
    client_size = sizeof(client_addr);              // initialize before accept
    while (externalCount < numExternals){

        // Accept an incoming connection:
        client_socket[externalCount] = accept(socket_desc, (struct sockaddr*)&client_addr, &client_size);
        if (client_socket[externalCount] < 0){
            printf("Can't accept\n");
            exit(0);
        }

        printf("One external process connected at IP: %s and port: %i\n",
               inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

        externalCount++; 
    }
    printf("--------------------------------------------------------------------------\n");
    printf("All four external processes are now connected\n");
    printf("--------------------------------------------------------------------------\n\n");

    // We intentionally don't close socket_desc here; keeping your original structure.
    return client_socket;   // Pointer to the array of file descriptors of client sockets 
}

int main(int argc, char *argv[])
{
    int socket_desc;    // (kept; we will not close this uninitialized FD at the end)

    // Messages received from clients (externals). 
    struct msg messageFromClient;   

    // Establish client connections and return an array of client sockets. 
    int * client_socket = establishConnectionsFromExternalProcesses(); 

    // Central initial temperature (keep main(void); read from env to avoid changing signature)
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <central_initial_temperature>\n", argv[0]);
        return 1;
    }
    float centralTemp = atof(argv[1]);

    printf("Central initial temperature = %.6f\n", centralTemp);

    // Track previous external temps for convergence test
    float prevTemps[numExternals];
    for (int i = 0; i < numExternals; i++) prevTemps[i] = 1e30f;  // large sentinel

    int stable = false;    // keep your name/type
    while ( !stable ){

        // Array that stores temperatures from clients 
        float temperature[numExternals];

        // Receive the messages from the 4 external processes 
        for (int i = 0;  i < numExternals; i++){
            if (recv(client_socket[i], (void *)&messageFromClient, sizeof(messageFromClient), 0) <= 0){
                printf("Couldn't receive\n");
                return -1;
            }
            temperature[i] = messageFromClient.T;
            printf("Temperature of External Process (%d) = %f\n", i, temperature[i]);
        }

        // Check stability condition: all externals changed < EPS since last iter
        int within = 0;
        for (int i = 0; i < numExternals; i++) {
            if (fabsf(temperature[i] - prevTemps[i]) < EPS) within++;
        }
        stable = (within == numExternals);

        // Modify central temperature per the assignment spec:
        // centralTemp <- (2*centralTemp + sum(externals))/6
        if (!stable) {                                             // only update if not stable
            float sumExt = 0.0f;
            for (int i = 0; i < numExternals; i++) sumExt += temperature[i];
            centralTemp = (2.0f * centralTemp + sumExt) / 6.0f;    // correct formula
        }

        // Construct message with updated temperature (+ done flag)
        struct msg updated_msg; 
        updated_msg.T = centralTemp;
        updated_msg.Index = 0;                // Index of central server 
        updated_msg.done = stable;

        // Send updated temps to the 4 external processes 
        for (int i = 0;  i < numExternals; i++){
            if (send(client_socket[i], (const void *)&updated_msg, sizeof(updated_msg), 0) < 0){
                printf("Can't send\n");
                return -1;
            }
        }

        printf("\n");

        // Prepare for next iteration
        for (int i = 0; i < numExternals; i++) prevTemps[i] = temperature[i];
    }

    // Closing all sockets
    for (int i = 0; i < numExternals; i++)
        close(client_socket[i]);

    return 0;
}
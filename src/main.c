#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include "./utils.h"

#define PORT 8080
#define BACKLOG 5

int main()
{
    struct sockaddr_in server_address;
    int server_socket, client_socket;
    // Create socket
    if ((server_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("Socket creation failed");
        return 1;
    }

    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = INADDR_ANY;
    server_address.sin_port = htons(PORT);

    // Bind socket to port
    if (bind(server_socket, (struct sockaddr *)&server_address, sizeof(server_address)) < 0)
    {
        perror("Bind failed");
        return 1;
    }

    // Start listening for incoming connections
    if (listen(server_socket, BACKLOG) < 0)
    {
        perror("Listen failed");
        return 1;
    }

    printf("Server started successfully!\n\n");

    printf("Local IP: 127.0.0.1:%d\n", PORT);
    printf("Server IP: %s:%d\n", get_current_ip(), PORT);

    while (1)
    {
        // Accept a new connection
        if ((client_socket = accept(server_socket, NULL, NULL)) < 0)
        {
            perror("Accept failed");
            continue; // Continue to accept next connection
        }

        // Display that a client has connected
        printf("Client connected\n");

        struct client_request_data data = receive_data(client_socket);

        send_data(client_socket, data);

        // Close the client socket
        close(client_socket);
    }

    // Close the server socket
    close(server_socket);

    return 0;
}
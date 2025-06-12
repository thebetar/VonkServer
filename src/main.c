#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include "./utils.h"
#include "./http.h"

int main()
{
    int client_socket;
    int server_socket = start_server();

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
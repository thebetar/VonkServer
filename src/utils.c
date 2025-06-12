#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include "utils.h"

#define PORT 8080
#define BACKLOG 5

char *get_current_ip()
{
    static char ip_str[INET_ADDRSTRLEN];
    int sock;
    struct sockaddr_in server_addr, local_addr;
    socklen_t addr_len = sizeof(local_addr);

    // Create a UDP socket
    sock = socket(AF_INET, SOCK_DGRAM, 0);

    if (sock < 0)
    {
        strcpy(ip_str, "127.0.0.1");
        return ip_str;
    }

    // Set up a remote address (Google's public DNS)
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(53);
    inet_pton(AF_INET, "8.8.8.8", &server_addr.sin_addr);

    // Connect to the remote address
    if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        close(sock);
        strcpy(ip_str, "127.0.0.1");
        return ip_str;
    }

    // Get the local address used for this connection
    if (getsockname(sock, (struct sockaddr *)&local_addr, &addr_len) < 0)
    {
        close(sock);
        strcpy(ip_str, "127.0.0.1");
        return ip_str;
    }

    // Convert the IP address to string
    inet_ntop(AF_INET, &local_addr.sin_addr, ip_str, INET_ADDRSTRLEN);

    close(sock);
    return ip_str;
}

int start_server()
{
    struct sockaddr_in server_address;
    int server_socket;
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

    return server_socket;
}

int tapo_fan_toggle(bool turn_on)
{
    // Call python script at "src/switch_tapo_plug.py" with argument "off" or "on"
    const char *command = turn_on ? "python3 src/switch_tapo_plug.py on" : "python3 src/switch_tapo_plug.py off";
    int result = system(command);

    if (result == 0)
    {
        printf("Tapo fan turned %s\n", turn_on ? "ON" : "OFF");
        return 0; // Return 0 for success
    }
    else
    {
        fprintf(stderr, "Failed to turn %s Tapo fan\n", turn_on ? "on" : "off");
        return 1; // Return 1 for failure
    }
}
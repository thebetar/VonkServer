#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define PORT 8080
#define BACKLOG 5

struct client_request_data
{
    int client_socket;
    char method[16];
    char body[1024];
};

int connection_count = 0;

char *get_headers(char *message)
{
    static char http_headers[512];
    sprintf(http_headers,
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: text/plain\r\n"
            "Content-Length: %d\r\n"
            "\r\n"
            "%s",
            (int)strlen(message), message);
    return http_headers;
}

char *get_data()
{
    // Read data from data.txt
    static char data[1024];
    FILE *file = fopen("data.txt", "r");

    if (file == NULL)
    {
        perror("Could not open data.txt");
        strcpy(data, "Error reading data");
        return data;
    }

    if (fgets(data, sizeof(data), file) == NULL)
    {
        strcpy(data, "Data file is empty");
    }

    fclose(file);

    return data;
}

char *post_data(char *data)
{
    // Write data to data.txt
    FILE *file = fopen("data.txt", "a+");

    if (file == NULL)
    {
        perror("Could not open data.txt for writing");
        return "Error writing data";
    }

    if (fputs(data, file) == EOF && fputs("\n", file) == EOF)
    {
        perror("Could not write data to data.txt");
        fclose(file);
        return "Error writing data";
    }

    fclose(file);
    return "Data written successfully";
}

struct client_request_data receive_data(int client_socket)
{
    char request_buffer[1024];
    int bytes_received = recv(client_socket, request_buffer, sizeof(request_buffer) - 1, 0);

    struct client_request_data result;
    result.client_socket = client_socket;

    if (bytes_received == 0)
    {
        strcpy(result.method, "GET");
        strcpy(result.body, "");
        return result; // No data received, return empty request
    }

    request_buffer[bytes_received] = '\0';

    char method[16];
    sscanf(request_buffer, "%15s", method);
    strcpy(result.method, method);

    printf("HTTP method: %s\n", method);

    char *body = strstr(request_buffer, "\r\n\r\n");

    if (body)
    {
        body += 4;
        strcpy(result.body, body);
        result.body[sizeof(result.body) - 1] = '\0'; // Ensure null termination
    }
    else
    {
        strcpy(result.body, "");
    }

    printf("Body: %s\n", body);

    return result;
}

void send_data(int client_socket, struct client_request_data request_data)
{
    // This function is not used in this example, but can be implemented if needed.
    // It could send data back to the client based on the request_data.
    printf("Sending data to client socket %d\n", client_socket);

    char *message;

    if (strcmp(request_data.method, "POST") == 0)
    {
        message = post_data(request_data.body);
    }
    else
    {
        message = get_data();
    }

    char *headers = get_headers(message);

    if (send(client_socket, headers, strlen(headers), 0) < 0)
    {
        perror("Send failed");
    }
    else
    {
        printf("Response sent to client socket %d\n", client_socket);
    }
}

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

    printf("Server started at port %d\n", PORT);

    while (1)
    {
        // Add connection count
        connection_count++;

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
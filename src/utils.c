#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include "database.h"
#include "utils.h"

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

struct client_request_data receive_data(int client_socket)
{
    char request_buffer[1024];
    // Receive data (MSG_WAITALL ensures we wait for all data since some clients send headers and body separately)
    int bytes_received = recv(client_socket, request_buffer, sizeof(request_buffer) - 1, 0);

    struct client_request_data result;
    result.client_socket = client_socket;

    strcpy(result.method, "GET"); // Default method
    strcpy(result.url, "");
    strcpy(result.body, "");

    if (bytes_received == 0)
    {
        return result; // No data received, return empty request
    }

    request_buffer[bytes_received] = '\0';

    char method[16];
    // Search for header line
    sscanf(request_buffer, "%15s", method);
    strcpy(result.method, method);
    printf("HTTP method: %s\n", method);

    char url[256];
    // Extract URL from the request
    sscanf(request_buffer, "%*s %255s", url);
    strcpy(result.url, url);
    printf("URL: %s\n", url);

    if (strcmp(method, "POST") != 0)
    {
        // If the method is not POST, we don't expect a body
        result.body[0] = '\0';
        return result;
    }

    char *body = strstr(request_buffer, "\r\n\r\n");

    if (body)
    {
        body += 4;

        // If body is empty but we expect data (POST request), try another recv (some clients send headers and body separately)
        if (strlen(body) == 0)
        {
            int additional_bytes = recv(client_socket, request_buffer + bytes_received,
                                        sizeof(request_buffer) - bytes_received - 1, 0);

            if (additional_bytes > 0)
            {
                request_buffer[bytes_received + additional_bytes] = '\0';
                body = strstr(request_buffer, "\r\n\r\n") + 4;
            }
        }

        strcpy(result.body, body);
        result.body[sizeof(result.body) - 1] = '\0'; // Ensure null termination
    }

    printf("Body: %s\n", result.body);

    return result;
}

void send_data(int client_socket, struct client_request_data request_data)
{
    // This function is not used in this example, but can be implemented if needed.
    // It could send data back to the client based on the request_data.
    printf("Sending data to client socket %d\n", client_socket);

    char *message;

    // Remove the first slash
    if (request_data.url[0] == '/')
    {
        memmove(request_data.url, request_data.url + 1, strlen(request_data.url));
    }

    // Check if there is still a slash left
    if (strchr(request_data.url, '/') != NULL)
    {
        // If there is a slash, return an error message
        message = "STATUS: Slash is not allowed in URL";
        printf("Error: %s\n", message);
        send(client_socket, get_headers(message), strlen(get_headers(message)), 0);
        return;
    }

    // Check if temperature or humidity
    if (strcmp(request_data.url, "temperature") != 0 && strcmp(request_data.url, "humidity") != 0)
    {
        message = "STATUS: Invalid URL";
        printf("Error: %s\n", message);
        send(client_socket, get_headers(message), strlen(get_headers(message)), 0);
        return;
    }

    // Check which HTTP method was used
    if (strcmp(request_data.method, "POST") == 0)
    {
        message = post_data(request_data.url, request_data.body);
    }
    else if (strcmp(request_data.method, "DELETE") == 0)
    {
        message = delete_data(request_data.url);
    }
    else if (strcmp(request_data.method, "GET") == 0)
    {
        message = get_data(request_data.url);
    }
    else
    {
        message = get_data(request_data.url);
    }

    // Prepare HTTP headers with the message
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
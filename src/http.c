#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include "database.h"
#include "http.h"
#include "utils.h"

#define MAX_REQUEST_SIZE 2048           // Maximum size for incoming HTTP request (2KiB)
#define MAX_RESPONSE_SIZE 1048576 * 2   // Maximum size for HTTP response (2MiB)

char *get_headers(char *message, char *content_type)
{
    // If content_type is empty, default to text/plain
    if (content_type == NULL || strlen(content_type) == 0)
    {
        content_type = "text/plain";
    }

    // Allocate 64KB for HTTP and content
    static char http_headers[MAX_RESPONSE_SIZE];

    // Check if content will be too large
    if (strlen(message) > MAX_RESPONSE_SIZE - 256)
    {
        fprintf(stderr, "Error: Message too large for HTTP response\n");
        return "HTTP/1.1 500 Internal Server Error\r\n\r\n";
    }

    sprintf(http_headers,
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: %s\r\n"
            "Content-Length: %d\r\n"
            "\r\n"
            "%s",
            content_type, (int)strlen(message), message);

    return http_headers;
}

struct client_request_data receive_data(int client_socket)
{
    char request_buffer[MAX_REQUEST_SIZE];
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

    char authorization[256];
    // Check for Authorization header
    if (strstr(request_buffer, "Authorization:") != NULL)
    {
        // Extract the Authorization header
        sscanf(strstr(request_buffer, "Authorization:") + 15, "%255s", authorization);
        printf("Authorization: %s\n", authorization);
    }

    if (strcmp(method, "POST") != 0 && strcmp(method, "PUT") != 0)
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

    printf("Processing request for URL: %s\n", request_data.url);

    // Check if there is still a slash left
    if (strchr(request_data.url, '/') != NULL)
    {
        // If there is a slash, return an error message
        message = "STATUS: Slash is not allowed in URL";
        printf("Error: %s\n", message);
        send(client_socket, get_headers(message, "text/plain"), strlen(get_headers(message, "text/plain")), 0);
        return;
    }

    // Check specific sensors URL for showing HTML page
    if (strcmp(request_data.url, "sensors") == 0)
    {
        FILE *file = fopen("templates/index.html", "r");

        if (file == NULL)
        {
            message = "STATUS: Could not open index.html";
            printf("Error: %s\n", message);
            send(client_socket, get_headers(message, "text/plain"), strlen(get_headers(message, "text/plain")), 0);
            return;
        }

        fseek(file, 0, SEEK_END);
        int file_size = ftell(file);
        fseek(file, 0, SEEK_SET);

        // Read the file content
        char *file_content = malloc(file_size + 1);

        if (file_content == NULL)
        {
            message = "STATUS: Memory allocation failed";
            printf("Error: %s\n", message);
            fclose(file);
            send(client_socket, get_headers(message, "text/plain"), strlen(get_headers(message, "text/plain")), 0);
            return;
        }

        int bytes_read = fread(file_content, 1, file_size, file);
        file_content[bytes_read] = '\0';
        fclose(file);

        send(client_socket, get_headers(file_content, "text/html"), strlen(get_headers(file_content, "text/html")), 0);

        free(file_content);
        return;
    }

    // Check if temperature or humidity
    if (
        strcmp(request_data.url, "temperature") != 0 &&
        strcmp(request_data.url, "humidity") != 0 &&
        strcmp(request_data.url, "light") != 0 &&
        strcmp(request_data.url, "air_quality") != 0 &&
        strcmp(request_data.url, "co"))
    {
        message = "STATUS: Invalid URL";
        printf("Error: %s\n", message);
        char *hearders = get_headers(message, "text/plain");
        send(client_socket, hearders, strlen(hearders), 0);
        return;
    }

    // Check which HTTP method was used
    if (strcmp(request_data.method, "POST") == 0)
    {
        message = post_data(request_data.url, request_data.body);

        // If temperature handle it
        if (strcmp(request_data.url, "temperature") == 0)
        {
            int temperature = atoi(request_data.body);
            handle_temperature(temperature);
        }

        // If humidity handle it
        if (strcmp(request_data.url, "humidity") == 0)
        {
            int humidity = atoi(request_data.body);
            handle_humidity(humidity);
        }
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
    char *headers = get_headers(message, "text/plain");

    if (send(client_socket, headers, strlen(headers), 0) < 0)
    {
        perror("Send failed");
    }
    else
    {
        printf("Response sent to client socket %d\n", client_socket);
    }
}
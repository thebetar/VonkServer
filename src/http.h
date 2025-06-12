struct client_request_data
{
    int client_socket;
    char method[16];
    char url[256];
    char body[1024];
};

struct client_request_data receive_data(int client_socket);
char *get_headers(char *message, char *content_type);
void send_data(int client_socket, struct client_request_data request_data);
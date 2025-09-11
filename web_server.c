#include "main.h"
#include "helpers.h"
#include "log_functions/logging.h"
#include "log_functions/log_local.h"


SOCKET create_socket (const char *host, const char *port) {
    printf ("Configuring local address...\n");
    struct addrinfo hints;
    memset (&hints, 0, sizeof (hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    struct addrinfo *bind_address;
    getaddrinfo (host, port, &hints, &bind_address);

    printf("Creating socket...\n");
    SOCKET socket_listen;
    socket_listen = socket(bind_address->ai_family,
                            bind_address->ai_socktype, bind_address->ai_protocol);

    if (!ISVALIDSOCKET(socket_listen)) {
        sprintf (log_buffer, "socket() failed. (%d)\n", GETSOCKETERRNO());
        log_error (1, log_buffer);
        exit(1);
    }

    printf("Binding socket to local address...\n");
    if (bind(socket_listen, bind_address->ai_addr, bind_address->ai_addrlen)) {
        sprintf (log_buffer, "bind() failed. (%d)\n", GETSOCKETERRNO());
        log_error (1, log_buffer);
        exit(1);
    }
    freeaddrinfo(bind_address);

    printf("Listening...\n");
    if (listen(socket_listen, 10) < 0) {
        sprintf (log_buffer,  "listen() failed. (%d)\n", GETSOCKETERRNO());
        log_error (1, log_buffer);
        exit(1);
    }
    return socket_listen;
}

int main(void)
{
    time_t current_time;

#if defined(_WIN32)
    WSADATA d;
    if (WSAStartup(MAKEWORD(2, 2), &d)) {
        log_error (1, "Failed to initialize WinSock.\n");
        return (1);
    }
#endif

    SOCKET server = create_socket(0, "8080");

    while (1) {
        fd_set reads;
        reads = wait_on_clients(server);

        if (FD_ISSET(server, &reads)) {
            client_info *client = get_client(-1);

            client->socket = accept (server, (struct sockaddr *) &(client->address), &(client->address_length));

            if (!ISVALIDSOCKET(client->socket)) {
                fprintf(stderr, "accept() failed. (%d)\n", GETSOCKETERRNO());
                return (1);
            }

            sprintf (log_buffer, "New connection from %s.\n", get_client_address(client));
            log_stdout (1, log_buffer);
        }

        client_info *client = clients;
        while (client) {
            client_info *next = client->next;

            if (FD_ISSET (client->socket, &reads)) {
                if (MAX_REQUEST_SIZE == client->received) {
                    send_400 (client, PATH_UNDTERMINED);
                    goto nextClient;
                }

                int r = recv (client->socket, client->request + client->received, MAX_REQUEST_SIZE - client->received, 0);
                if (r < 1) {
                    sprintf (log_buffer, "Unexpected disconnect from %s.\n", get_client_address(client));
                    log_stdout (1, log_buffer);
                    drop_client (client);
                } else {
                    client->received += r;
                    client->request[client->received] = 0;

                    char *q = strstr (client->request, "\r\n\r\n");

                    if (q) {
                        if (strncmp("GET /", client->request, 5)) {
                            send_400(client, PATH_UNDTERMINED);
                        } else {
                            char *path = client->request + 4;
                            char *end_path = strstr(path, " ");
                            if (!end_path) {
                                send_400(client, PATH_UNDTERMINED);
                            } else {
                                *end_path = 0;
                                serve_resource (client, path);
                            }
                        }
                    }
                }
            }
            nextClient:
                client = next;
        }
    }

    printf ("\nClosing socket...\n");
    CLOSESOCKET (server);

#if defined(_WIN32)
    WSACleanup();
#endif
    printf("Finished.\n");
    return 0;    
}
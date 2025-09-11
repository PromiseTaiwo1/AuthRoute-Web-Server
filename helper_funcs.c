#include "main.h"
#include "log_functions/logging.h"
#include "log_functions/log_local.h"

// Global struct variable to deal with connected clients
client_info *clients = NULL;

const char *get_content_type(const char* path) {
    const char *last_dot = strrchr(path, '.');
    if (last_dot) {
    if (strcmp(last_dot, ".css") == 0) return "text/css";
    if (strcmp(last_dot, ".csv") == 0) return "text/csv";
    if (strcmp(last_dot, ".gif") == 0) return "image/gif";
    if (strcmp(last_dot, ".htm") == 0) return "text/html";
    if (strcmp(last_dot, ".html") == 0) return "text/html";
    if (strcmp(last_dot, ".ico") == 0) return "image/x-icon";
    if (strcmp(last_dot, ".jpeg") == 0) return "image/jpeg";
    if (strcmp(last_dot, ".jpg") == 0) return "image/jpeg";
    if (strcmp(last_dot, ".js") == 0) return "application/javascript";
    if (strcmp(last_dot, ".json") == 0) return "application/json";
    if (strcmp(last_dot, ".png") == 0) return "image/png";
    if (strcmp(last_dot, ".pdf") == 0) return "application/pdf";
    if (strcmp(last_dot, ".svg") == 0) return "image/svg+xml";
    if (strcmp(last_dot, ".txt") == 0) return "text/plain";
    }
    return "application/octet-stream";
}


/**
  * get_client - gets a client from a linked list or adds a new
  * client if none is found.
  * s: the connection socket of the client to find or add.
  * Return: the searched client (if found) or a new client created (if search fails).
*/
client_info *get_client (SOCKET s) {
    client_info *ci = clients;
    client_info *n; // pointer to a new client_info

    while (ci) {
        if (ci->socket == s) break;
        ci = ci->next;
    }

    if (ci) return (ci);

    // Since we can't find the requested client/socket,
    // we add a new client to the linked list.
    n = (client_info *) calloc (1, sizeof (struct client_info));
    if (n == NULL) {
        log_error (1, "Out of dynamic memory.\n");
        exit (1);
    }

    n->address_length = sizeof (n->address);
    n->next = clients;
    clients = n;
    return (n);
}

void drop_client (client_info *client) {
    client_info **p = &clients;

    // Close the client connection or socket.
    CLOSESOCKET (client->socket);

    while (*p) {
        if (*p == client) {
            *p = client->next;
            free (client);
            return;
        }
        p = &(*p)->next;
    }

    log_error (1, "drop_client failed. client not found.\n");
    exit (1);
}

const char *get_client_address (client_info *ci) {
    static char address_buffer[100];
    getnameinfo ((struct sockaddr *) &ci->address, ci->address_length,
                 address_buffer, sizeof (address_buffer), 0, 0, NI_NUMERICHOST);
    return (address_buffer);
}

fd_set wait_on_clients(SOCKET server) {
    fd_set reads;
    SOCKET max_socket;
    client_info *ci;
    
    FD_ZERO (&reads);
    FD_SET (server, &reads);
    max_socket = server;
    ci = clients;
    while (ci) {
        FD_SET (ci->socket, &reads);
        if (ci->socket > max_socket) max_socket = ci->socket;
        ci = ci->next;
    }

    if (select (max_socket + 1, &reads, 0, 0, 0) < 0) {
        snprintf (log_buffer, LOG_BUFFER_SIZE - 1, "select() failed. (%d)\n", GETSOCKETERRNO ());
        log_error (1, log_buffer);
        exit (1);
    }

    return (reads);
}

void send_400(client_info *client, const char *path) {
    const char *c400 = "HTTP/1.1 400 Bad Request\r\n"
                       "Connection: close\r\n"
                       "Content-Length: 11\r\n"
                       "Content-Type: text/plain\r\n\r\n"
                       "Bad Request";
    send (client->socket, c400, strlen (c400), 0);
    log_stdout (2, path, "400 Bad Request\n");
    drop_client (client);
}

void send_404 (client_info *client, const char *path) {
    const char *c404 = "HTTP/1.1 404 Not Found\r\n"
                       "Connection: close\r\n"
                       "Content-Length: 9\r\n"
                       "Content-Type: text/plain\r\n\r\n"
                       "Not Found";
    send (client->socket, c404, strlen (c404), 0);
    log_stdout (2, path, "404 Not Found\n");
    drop_client (client);
}


void serve_resource (client_info *client, const char *path) {
#define PATH_SIZE 128
    char full_path[PATH_SIZE];
    char *p;
    FILE *fp;
    size_t cl; // file content length variable
    const char *ct; // file content type variable

    // Add this logging only if verbose is specified.
    // log_stdout (4, "serve_resourse", get_client_address (client), path, "\n");

    if (strcmp (path, "/") == 0) path = "/index.html";

    if (strlen (path) > 100) {
        send_400 (client, path);
        return;
    }

    if (strstr (path, "..")) {
        send_404 (client, path);
        return;
    }


    snprintf (full_path, PATH_SIZE, "public%s", path);
// TO handle difference in directory separator between Windows and Unix
#if defined(_WIN32)
    p = full_path;
    while (*p) {
        if (*p == '/') *p = '\\';
        ++p;
    }
#endif

    fp = fopen (full_path, "rb");

    if (!fp) {
        send_404 (client, path);
        return;
    }

    fseek (fp, 0L, SEEK_END);
    cl = ftell (fp);
    rewind (fp);

    ct = get_content_type (full_path);

#define BSIZE 1024
    char buffer[BSIZE];

    sprintf (buffer, "HTTP/1.1 200 OK\r\n");
    send (client->socket, buffer, strlen (buffer), 0);

    sprintf (buffer, "Connection: close\r\n");
    send (client->socket, buffer, strlen (buffer), 0);

    sprintf (buffer, "Content-Length: %zu\r\n", cl);
    send (client->socket, buffer, strlen (buffer), 0);

    sprintf (buffer, "Content-Type: %s\r\n", ct);
    send (client->socket, buffer, strlen (buffer), 0);

    sprintf (buffer, "\r\n");
    send (client->socket, buffer, strlen (buffer), 0);

    // TODO: Create a separate thread or process for each client
    // before sending the files for situations when the file is
    // soo large and it can cause blocking of other clients.
    while (1) {
        int read = fread (buffer, 1, BSIZE, fp);
        if (read <= 0) break;
        send (client->socket, buffer, read, 0);
    }
    fclose (fp);
    log_stdout (2, path, "200 Ok\n");
    drop_client (client);
}

// End of file
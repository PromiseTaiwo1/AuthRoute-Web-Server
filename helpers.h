#ifndef HELPERS_H
#define HELPERS_H

const char *get_content_type(const char* path);

client_info *get_client (SOCKET s);
void drop_client (client_info *client);
const char *get_client_address (client_info *ci);
fd_set wait_on_clients(SOCKET server);
void serve_resource (client_info *client, const char *path);


// Error handling functions
void send_400(client_info *client, const char *path);
void send_404 (client_info *client, const char *path);

extern struct client_info *clients;

#endif
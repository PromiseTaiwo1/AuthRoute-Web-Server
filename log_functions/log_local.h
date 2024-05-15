#ifndef LOGS_H
#define LOGS_H

#define LOG_BUFFER_SIZE 1024


typedef struct server_config {
    char *log_file;
} server_config;

static char log_buffer[LOG_BUFFER_SIZE];

#endif
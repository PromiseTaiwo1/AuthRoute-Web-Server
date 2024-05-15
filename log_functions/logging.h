#ifndef LOGGING_H
#define LOGGING_H

void log_stdout(int count, ...);
void log_error(int count, ...);
void log_file(int count, char *msg, ...);

#endif
#include <time.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "log_local.h"



server_config *read_config() {
    FILE *conf_fp;
    server_config *config_details = malloc (sizeof (server_config));
    char line[100];
    char *log_file = NULL;

    conf_fp = fopen ("authroute.conf", "r");
    if (fgets (line, sizeof (line), conf_fp)) {
        log_file = strtok (line, "LOG_FILE ");
    }

    config_details->log_file = malloc (sizeof (char) * strlen (log_file) + 1);
    strcpy (config_details->log_file, log_file);

    // Cleanup
    fclose (conf_fp);
    return (config_details);
}

void log_file (int count, char *msg, ...) {
// Change the code in this function. The log file should be opened at the top
//  level to avoid openign it at each call to this function. THat way the log file
// can be written to continously as long as the program is running while avoiding overhead.
// Closing the fie can come as part of a cleanup operation that checks for terminal signals.

    server_config *conf;
    FILE *log_file;

    conf = read_config ();
    log_file = fopen (conf->log_file, "a");

    if (!log_file)
    {
        perror ("Error opening log file");
        return;
    }

    fprintf (log_file, msg);

    // Cleanup.
    fclose (log_file);
    free (conf->log_file);
    free (conf);
}

void log_stdout(int count, ...) {
    time_t current_time;
    char *curr_time, *newline_ptr, *msg;
    va_list args;
    static int calls_count = 0;
    size_t written = 0;

    va_start (args, count);

    // Print out the current time first.
    time (&current_time);
    curr_time = ctime (&current_time);
    
    // Remove the newline character that comes with ctime()
    if ((newline_ptr = strchr(curr_time, '\n')) != NULL) {
        *newline_ptr = '\0';
    }

    sprintf (log_buffer, "[%s] ", curr_time);
    written = strlen (log_buffer);
    
    for (int i = 0; i < count; i++)
    {
        msg = va_arg (args, char *);
        if (i < count - 1)
            sprintf (log_buffer + written, "%s ", msg);
        else
            sprintf (log_buffer + written, "%s", msg);
        written = strlen (log_buffer);
    }

    printf (log_buffer);
    log_file (0, log_buffer);

    va_end (args);
}

void log_error(int count, ...) {
    time_t current_time;
    char *curr_time, *newline_ptr, *msg;
    va_list args;
    static int calls_count = 0;
    size_t written = 0;

    va_start (args, count);

    // Print out the current time first.
    time (&current_time);
    curr_time = ctime (&current_time);
    
    // Remove the newline character that comes with ctime()
    if ((newline_ptr = strchr(curr_time, '\n')) != NULL) {
        *newline_ptr = '\0';
    }

    sprintf (log_buffer, "[%s] ", curr_time);
    written = strlen (log_buffer);
    
    for (int i = 0; i < count; i++)
    {
        msg = va_arg (args, char *);
        if (i < count - 1)
            sprintf (log_buffer + written, "%s ", msg);
        else
            sprintf (log_buffer + written, "%s", msg);
        written = strlen (log_buffer);
    }

    fprintf (stderr, log_buffer);
    log_file (0, log_buffer);

    va_end (args);
}


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <time.h>
#include <errno.h>

#include "persist.h"

static int
get_persist_filepath(uid_t user_id, char *path_buf, size_t buf_len)
{
        int n = snprintf(path_buf, buf_len, "%s/%d", PERSIST_DIR, (int)user_id);
        if (n < 0 || (size_t)n >= buf_len) {
                fprintf(stderr, "persist: Failed to construct file path\n");
                return -1;
        }
        return 0;
}

int
check_persist(uid_t user_id)
{
        char filepath[256];
        if (get_persist_filepath(user_id, filepath, sizeof(filepath)) != 0) return 0; /* cannot construct path */

        struct stat st;
        if (stat(filepath, &st) != 0) return 0; /* file doesn't exist or stat error, persist not active */

        /*
        if (st.st_uid != user_id || (st.st_mode & 0777) != 0600) {
                fprintf(stderr, "persist: Warning: invalid permissions on ownership on %s\n", filepath);
                return 0;
        }
        */
        

        int fd = open(filepath, O_RDONLY);

        char timestamp_str[32];
        ssize_t bytes_read = read(fd, timestamp_str, sizeof(timestamp_str) - 1);
        close(fd);

        timestamp_str[bytes_read] = '\0'; /* null terminate */

        errno = 0;
        time_t stored_time = (time_t)strtol(timestamp_str, NULL, 10);
        if (errno != 0 || stored_time <= 0) {
                fprintf(stderr, "persist: Warning: invalid timestamp format in %s\n", filepath);
                unlink(filepath); /* remove corrupt file */
                return 0;
        }

        time_t current_time = time(NULL);
        if (current_time < 0) {
                perror("persist: time failed");
                return 0; /* can not get current time */
        }

        if ((current_time - stored_time) <= PERSIST_TIMEOUT) {
                return 1; /* timestamp is valid within the timeout period */
        } else {
                unlink(filepath);
                return 0; /* timestamp expired*/
        }
}

int
update_persist(uid_t user_id)
{
    struct stat st;
    int stat_ret = stat(PERSIST_DIR, &st);
    int saved_errno = errno;

    if (stat_ret == -1) {
        if (saved_errno == ENOENT) { /* directory just doesn't exist */
            if (mkdir(PERSIST_DIR, 0777) == -1) {
                perror("persist: Failed to create directory");
                fprintf(stderr, "persist: Check permissions for parent of %s\n", PERSIST_DIR);
                return -1;
            }
            if (chmod(PERSIST_DIR, 01777) == -1) {
                 perror("persist: Failed to set permissions on directory");
                 return -1;
            }
            if (chown(PERSIST_DIR, 0, 0) == -1) {
                 perror("persist: Failed to set ownership on directory");
                 return -1;
            }
             fprintf(stderr, "persist: Info: Created directory %s\n", PERSIST_DIR);
        } else {
            fprintf(stderr, "persist: Failed to stat persistence directory '%s': %s\n",
                    PERSIST_DIR, strerror(saved_errno));
            return -1;
        }
    } else { /* stat succeeded - path exist */
        if (!S_ISDIR(st.st_mode)) {
            fprintf(stderr, "persist: Error: %s exists but is not a directory.\n", PERSIST_DIR);
            return -1;
        }
    }
    char filepath[256];
    if (get_persist_filepath(user_id, filepath, sizeof(filepath)) != 0) {
        return -1;
    }

    time_t current_time = time(NULL);
    if (current_time < 0) {
        perror("persist: time failed");
        return -1;
    }

    char timestamp_str[32];
    int n = snprintf(timestamp_str, sizeof(timestamp_str), "%ld", (long)current_time);
    if (n < 0 || (size_t)n >= sizeof(timestamp_str)) {
        fprintf(stderr, "persist: Failed to format timestamp\n");
        return -1;
    }

    int fd = open(filepath, O_WRONLY | O_CREAT | O_TRUNC, 0600);
    if (fd < 0) {
        perror("persist: Failed to open timestamp file for writing");
        fprintf(stderr, "persist: Check permissions within %s for user %d\n", PERSIST_DIR, (int)user_id);
        return -1;
    }

    ssize_t bytes_written = write(fd, timestamp_str, strlen(timestamp_str));
    saved_errno = errno;
    int close_ret = close(fd);
    int write_error = (bytes_written < (ssize_t)strlen(timestamp_str));

    if (close_ret != 0) {
         perror("persist: Failed to close timestamp file");
         if (write_error) {
             fprintf(stderr, "persist: Also failed to write full timestamp: %s\n", strerror(saved_errno));
             unlink(filepath);
         }
         return -1;
    }

    if (write_error) {
        fprintf(stderr, "persist: Failed to write full timestamp: %s\n", strerror(saved_errno));
        unlink(filepath);
        return -1;
    }

    if (chown(filepath, user_id, -1) == -1) { /* change owner to user_id, keep group (-1 means don't change) */
        perror("persist: Failed to set ownership on timestamp file");
        unlink(filepath);
        return -1;
    }

    return 0;
}

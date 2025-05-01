#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pwd.h>
#include <grp.h>
#include <sys/stat.h>
#include <time.h>
#include <crypt.h>
#include <errno.h>

#ifdef PERSIST
#include "persist.h"
#endif /* PERSIST */

#define CONFIG_FILE "/etc/lx.conf"
#define DEFAULT_CONFIG "# lx configuration file\n" \
                      "# Uncomment lines to grant permissions\n" \
                      "#permit :wheel\n" \
                      "permit root\n"

struct passwd *pw; /* forward declaration for is_user_in_group dependency */

void create_default_config() {
    FILE *fp = fopen(CONFIG_FILE, "w");
    if (fp == NULL) {
        perror("Failed to create config file");
        exit(1);
    }
    fputs(DEFAULT_CONFIG, fp);
    fclose(fp);
    chmod(CONFIG_FILE, 0640);
}

char *get_shadow_password(const char *username) {
    FILE *fp = fopen("/etc/shadow", "r");
    if (fp == NULL) return NULL;

    char line[1024];
    char *password = NULL; /* initialize password to NULL */

    while (fgets(line, sizeof(line), fp) != NULL) {
        char *token = strtok(line, ":");
        if (token == NULL) continue;
        if (strcmp(token, username) == 0) {
            token = strtok(NULL, ":");
            if (token != NULL && strcmp(token, "*") != 0 && strcmp(token, "!") != 0) { /* check for actual password hash */
                password = strdup(token);
                if (!password) {
                        perror("strdup failed");
                        break; /* keep searching? or exit? to search or not to search, but for simplicity, break here */
                }
                break; /* no need for strcspn, hash includes salt/info, keep it all. */
            }
        }
    }
    fclose(fp);
    return password; /* can be NULL if user not found or no pass */
}

int is_user_in_group(const char *username, const char *groupname) {
    struct group *grp = getgrnam(groupname);
    if (grp == NULL) return 0;

    /* use global pw var (ensures we check against the right user) */
    if (pw == NULL) {
            fprintf(stderr, "Error: User information not available for group check.\n");
            return 0;
    }

    if (pw->pw_gid == grp->gr_gid) return 1;

    for (char **member = grp->gr_mem; *member != NULL; member++) {
        if (strcmp(*member, username) == 0) {
            return 1;
        }
    }
    return 0;
}

int check_permission(const char *username) {
    FILE *conf = fopen(CONFIG_FILE, "r");
    if (conf == NULL) {
        if (getuid() == 0) {
            create_default_config();
            conf = fopen(CONFIG_FILE, "r");
            if (conf == NULL) {
                perror("Failed to open config file");
                return 0;
            }
        } else {
            perror("Failed to open config file");
            return 0;
        }
    }

    /* check config file permissions and ownership */
    struct stat st;
    if (fstat(fileno(conf), &st) == 0) {
            /* should be owned by root, group root or wheel maybe? let's stick to root:root. */
            if (st.st_uid != 0 || (st.st_mode & S_IWOTH)) {
                    fprintf(stderr, "Warning: Insecure permissions or ownership on %s\n", CONFIG_FILE);
                    fprintf(stderr, "Please make config file owned by root and not world writtable (chmod 0644 or 0640)\n");
            }
    } else {
         perror("Warning: Could not stat config file");
    }

    int allowed = 0;
    char line[256];
    while (fgets(line, sizeof(line), conf) != NULL) {
        line[strcspn(line, "\n")] = 0;
        if (line[0] == '#' || line[0] == '\0') continue;

        char action[16], target[64];
        if (sscanf(line, "%15s %63s", action, target) >= 2 && 
            strcmp(action, "permit") == 0) {
            // Check group permission
            if (target[0] == ':') {
                if (is_user_in_group(username, target+1)) {
                    allowed = 1;
                    break;
                }
            }
            // Check user permission
            else if (strcmp(username, target) == 0) {
                allowed = 1;
                break;
            }
        }
    }
    fclose(conf);
    return allowed;
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s command [args...]\n", argv[0]);
        return 1;
    }

    uid_t real_uid = getuid();
    pw = getpwuid(real_uid);
    if (pw == NULL) {
        perror("getpwuid");
        return 1;
    }

#ifdef PERSIST
    int persist_active = check_persist(real_uid);
        if (persist_active) {
                if (update_persist(real_uid) != 0) {
                        fprintf(stderr, "persist: Warning: Failed to update timestamp after successful check.\n");
                }
                goto execute_command;
        }
#endif /* PERSIST */

    if (!check_permission(pw->pw_name)) {
        fprintf(stderr, "Permission denied\n");
        return 1;
    }

    char *stored_password = get_shadow_password(pw->pw_name);
    if (stored_password == NULL) {
        fprintf(stderr, "Authentication failed\n");
        return 1;
    }

    char *entered_password = getpass("Password: ");
    if (!entered_password) {
        fprintf(stderr, "Failed to read password\n");
        free(stored_password);
        return 1;
    }

    char *crypt_password = crypt(entered_password, stored_password);
    memset(entered_password, 0, strlen(entered_password)); /* zero out immediately after use */
    if (!crypt_password || strcmp(crypt_password, stored_password) != 0) {
        fprintf(stderr, "Authentication failed\n");
        free(stored_password);
        return 1;
    }

    free(stored_password);

#ifdef PERSIST
    if (update_persist(real_uid) != 0) {
            fprintf(stderr, "persist: Warning: Failed to update timestamp after password authentication.\n");
    }

execute_command:    
#endif /* PERSIST */
    execvp(argv[1], &argv[1]);
    perror("execvp");
    return 127; /* standard exit code for command not found/exec error */
}

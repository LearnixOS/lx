#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pwd.h>
#include <grp.h>
#include <sys/stat.h>
#include <time.h>

#define CONFIG_FILE "/etc/lx.conf"
#define DEFAULT_CONFIG "# lx configuration file\n" \
                      "# Uncomment lines to grant permissions\n" \
                      "#permit :wheel\n" \
                      "permit root\n"

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
    while (fgets(line, sizeof(line), fp) != NULL) {
        char *token = strtok(line, ":");
        if (token == NULL) continue;
        if (strcmp(token, username) == 0) {
            token = strtok(NULL, ":");
            if (token != NULL) {
                char *password = strdup(token);
                password[strcspn(password, "\n")] = 0;
                fclose(fp);
                return password;
            }
        }
    }
    fclose(fp);
    return NULL;
}

int is_user_in_group(const char *username, const char *groupname) {
    struct group *grp = getgrnam(groupname);
    if (grp == NULL) return 0;

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

    struct passwd *pw = getpwuid(getuid());
    if (pw == NULL) {
        perror("getpwuid");
        return 1;
    }

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
    if (!crypt_password || strcmp(crypt_password, stored_password) != 0) {
        fprintf(stderr, "Authentication failed\n");
        free(stored_password);
        return 1;
    }

    free(stored_password);
    execvp(argv[1], &argv[1]);
    perror("execvp");
    return 1;
}

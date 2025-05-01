#ifndef PERSIST_H
#define PERSIST_H

#include <sys/types.h>
#include <time.h>

#define PERSIST_DIR "/var/run/lx"

/* Persist timeout in seconds */
#define PERSIST_TIMEOUT 30

/*
 * @brief Checks if a valid persistence timestamp exists for the user.
 * @param user_id The real user ID to check persistence for.
 * @return 1 if a valid timestamp exists (within PERSIST_TIMEOUT), 0 otherwise.
 */
int check_persist(uid_t user_id);

/*
 * @brief Updates or creates the persistence timestamp for the user.
 * @param user_id The real user ID to update the timestamp for.
 * @return 0 on success, -1 on failure.
 */
int update_persist(uid_t user_id);

#endif /* PERSIST_H */

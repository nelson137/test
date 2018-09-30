#include "lssh-server.c"

char *query_conf(char *host, char *key) {
    // Wait for the lock to go away if it exists
    do {
        if (access(CONF_LOCK_FN, R_OK) == -1)
            break;
        else
            sleep(1);
    } while(1);

    // Create a string of the command to be executed
    char cmd_s[13+strlen(host)+strlen(key)+strlen(CONF_FN)];
    sprintf(cmd_s, "jq -rj '.%s.%s' %s", host, key, CONF_FN);

    // Execute the command
    FILE *cmd = popen(cmd_s, "r");
    if (cmd == NULL)
        err("Failed to run jq\n", 1);

    // Read jq output
    char *output = NULL;
    size_t o_size;
    ssize_t o_bytes_read = getdelim(&output, &o_size, '\0', cmd);
    if (o_bytes_read == -1)
        err("Could not read output of jq command\n", 1);

    pclose(cmd);

    return output;
}

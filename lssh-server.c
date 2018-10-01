#define _DEFAULT_SOURCE

#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <arpa/inet.h>

#define SERVER_PORT 1500
#define MAX_MSG_LEN 100


char *CWD = NULL;
char *CONF_FN = NULL;
char *CONF_LOCK_FN = NULL;


void err(char *msg, int code) {
    fprintf(stderr, "%s", msg);
    exit(code);
}


// Get current working directory
char *get_cwd(void) {
    int cwd_size = 100;
    char *cwd = malloc(sizeof(char*) * cwd_size);
    while (getcwd(cwd, cwd_size-1) == NULL) {
        cwd_size *= 2;
        cwd = realloc(cwd, cwd_size);
    }

    return cwd;
}


char *join_strings(char *first, char* delim, char *second) {
    int size = strlen(first) + strlen(delim) + strlen(second) + 1;
    char *joined = malloc(sizeof(char) * size);
    sprintf(joined, "%s%s%s", first, delim, second);
    return joined;
}


void update_conf(char *host, char *key, char *new_val) {
    // Create a string of the command to be executed
    int size = 31 + strlen(host) + strlen(key) + strlen(new_val) + \
               2*strlen(CONF_FN);
    char *cmd_s = malloc(sizeof(char) * size);
    char *cmd_s_fmt = "jq -r '.\"%s\".\"%s\" = \"%s\"' %s | sponge %s";
    sprintf(cmd_s, cmd_s_fmt, host, key, new_val, CONF_FN, CONF_FN);

    // Execute the command
    FILE *cmd = popen(cmd_s, "r");
    if (cmd == NULL)
        err("Failed to run jq to update the conf file\n", 1);

    pclose(cmd);
}


void logger(char *severity, char *fmt, ...) {
    // Print the message's severity
    printf("[%s] ", severity);

    va_list args;
    va_start(args, fmt);

    // Print the message, replacing format specifiers with va_args
    while(*fmt != '\0') {
        if (*fmt == '%' && *(fmt+1) != '\0' && isalpha(*(fmt+1))) {
            fmt++;
            switch (*fmt) {
                case 'd':
                    printf("%d", va_arg(args, int));
                    break;
                case 's':
                    printf("%s", va_arg(args, char*));
                    break;
                default:
                    printf("\nformat specifier not recognized: %c\n", *fmt);
                    break;
            }
        } else {
            printf("%c", *fmt);
        }

        fmt++;
    }

    fflush(stdout);

    va_end(args);
}


void setup(void) {
    CWD = get_cwd();
    if (CWD == NULL)
        err("Could not get current working directory\n", 1);

    CONF_FN = join_strings(CWD, "/", "connections.json");
    if (CONF_FN == NULL)
        err("Could not get path of conf file\n", 1);

    CONF_LOCK_FN = join_strings(CONF_FN, ".", "lock");
    if (CONF_LOCK_FN == NULL)
        err("Could not get path of conf lock file\n", 1);
}


int main(void) {
    setup();

    int ret;
    int broadcast = 1;

    // Create socket
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        logger("ERROR", "cannot open socket\n");
        exit(1);
    }

    // Set broadcast permission on socket
    ret = setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &broadcast,
                     sizeof broadcast);

    if (ret == -1) {
        perror("setsockopt (SO_BROADCAST)");
        exit(1);
    }

    // Bind to server port
    struct sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serverAddr.sin_port = htons(SERVER_PORT);
    ret = bind(sock, (struct sockaddr *) &serverAddr,sizeof(serverAddr));

    if (ret < 0) {
        logger("ERROR", "cannot bind port %d\n", SERVER_PORT);
        exit(1);
    }

    // Listen for messages
    socklen_t cliLen;
    char msg[MAX_MSG_LEN];
    while(1) {
        // Initialize buffer
        memset(msg, 0x0, MAX_MSG_LEN);

        // Receive message
        struct sockaddr_in clientAddr;
        cliLen = sizeof(clientAddr);
        ret = recvfrom(sock, msg, MAX_MSG_LEN, 0, (struct sockaddr*) &clientAddr,
                       &cliLen);

        if (ret < 0) {
            logger("ERROR", "cannot get message\n");
            continue;
        }

        // Log received message
        char *ip = inet_ntoa(clientAddr.sin_addr);
        logger("INFO", "%s new internal ip: %s\n", msg, ip);
        update_conf(msg, "internal_ip", ip);
    }

    return 0;
}

#define _GNU_SOURCE
#include <math.h>    // log10()
#include <stdarg.h>  // va_list, va_start(), va_arg(), va_end()
#include <stdio.h>   // printf()
#include <stdlib.h>  // exit()
#include <string.h>  // memset()
#include <time.h>    // time_t, tm, localtime_r()

#include <arpa/inet.h>  // socklen_t, socket(), AF_INET, SOCK_DGRAM, htonl(),
                        // INADDR_ANY, htons(), bind(), recvfrom(),
                        // inet_ntoa(), ntohs

#define SERVER_PORT 1500
#define MAX_MSG_LEN 100


void logger(char *severity, char *fmt, ...) {
    // Print the current date and time
    time_t t = time(NULL);
    struct tm lt = {0};
    localtime_r(&t, &lt);
    printf("%d-%02d-%02d %02d:%02d:%02d %s [%s] ",
           lt.tm_year+1900, lt.tm_mon+1, lt.tm_mday,      // Date
           lt.tm_hour, lt.tm_min, lt.tm_sec, lt.tm_zone,  // Time
           severity);                                     // Tag

    va_list args;
    va_start(args, fmt);

    while(*fmt != '\0') {
        if (*fmt == '%' && *(fmt+1) != '\0') {
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


int main(void) {
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
        logger("INFO", "received message from %s: %s\n", ip, msg);
    }

    return 0;
}

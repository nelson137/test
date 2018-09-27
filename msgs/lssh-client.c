#define _DEFAULT_SOURCE

#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define SERVER_ADDRESS "255.255.255.255"
#define SERVER_PORT    1500


char *getHostname(void) {
    int hn_size = 32;
    char *hn = malloc(sizeof(char) * hn_size);
    while (gethostname(hn, hn_size) < 0) {
        hn_size *= 2;
        if (hn_size > 512) {
            puts("hostname is too long");
            exit(1);
        }
        hn = (char*) realloc(hn, sizeof(char) * hn_size);
    }

    return hn;
}


int main(int argc, char *argv[]) {
    int ret;
    int broadcast = 1;

    // Server address
    struct sockaddr_in serverAddr;
    struct hostent *h = gethostbyname(SERVER_ADDRESS);
    serverAddr.sin_family = h->h_addrtype;
    memcpy((char*) &serverAddr.sin_addr.s_addr,
           h->h_addr_list[0], h->h_length);
    serverAddr.sin_port = htons(SERVER_PORT);

    // Create the socket
    int sock = socket(AF_INET,SOCK_DGRAM, 0);
    if (sock < 0) {
        printf("%s: cannot open socket\n", argv[0]);
        exit(1);
    }

    // Set broadcast permission on socket
    ret = setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &broadcast,
                     sizeof broadcast);

    if (ret == -1) {
        perror("setsockopt (SO_BROADCAST)");
        exit(1);
    }

    // Bind to any port
    struct sockaddr_in clientAddr;
    clientAddr.sin_family = AF_INET;
    clientAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    clientAddr.sin_port = htons(0);
    ret = bind(sock, (struct sockaddr*) &clientAddr, sizeof(clientAddr));

    if (ret < 0) {
        printf("%s: cannot bind port\n", argv[0]);
        exit(1);
    }

    // Send message
    char *msg = getHostname();
    ret = sendto(sock, msg, strlen(msg)+1, 0,
                 (struct sockaddr*) &serverAddr,
                 sizeof(serverAddr));

    if (ret < 0) {
        printf("%s: cannot send message\n", argv[0]);
        close(sock);
        exit(1);
    }

    return 0;
}

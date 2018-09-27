#define _DEFAULT_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <arpa/inet.h>

#include <mylib.h>


char *get_default_gateway(void) {
    FILE *f;
    char line[100], *iface, *dest, *gw, *end;
    char *gateway = NULL;

    f = fopen("/proc/net/route", "r");

    while (fgets(line, 100, f)) {
        iface = strtok_r(line, " \t", &end);
        dest = strtok_r(NULL, " \t", &end);
        gw = strtok_r(NULL, " \t", &end);

        if (iface!=NULL && dest!=NULL && strcmp(dest,"00000000")==0) {
            if (gw) {
                char *pEnd;
                // Convert gw to an int
                int ng = strtol(gw, &pEnd, 16);
                // Convert the gw bits to dotted-decimal notation
                struct in_addr addr;
                addr.s_addr = ng;
                gateway = inet_ntoa(addr);
            }
            break;
        }
    }

    fclose(f);
    return gateway;
}


void attempt_ssh(char *host) {
    char *gateway = get_default_gateway();
    printf("default gateway: %s\n", gateway);
    // Execute the ssh command
    execl("/usr/bin/ssh", "ssh", host, (char*)NULL);
}


int main (void) {
    int max_lines = 10;
    int max_line_len = 100;

    // Allocate memory for connections
    char **lines = (char**)malloc(sizeof(char*) * max_lines);
    if (lines == NULL) {
        fprintf(stderr, "Out of memory\n");
        exit(1);
    }

    // Attempt to open the connections file
    char *conns_f = "connections.csv";
    FILE *fp = fopen(conns_f, "r");
    if (fp == NULL) {
        fprintf(stderr, "Cannot open file %s", conns_f);
        exit(4);
    }

    int consumed_first = 0;
    int nl;
    for (nl=0; 1; nl++) {
        // Re-allocate if max_lines has been reached
        if (nl >= max_lines) {
            max_lines = max_lines * 2;
            lines = (char**)realloc(lines, sizeof(char*) * max_lines);
            if (lines == NULL) {
                fprintf(stderr, "Out of memory: too many lines\n");
                exit(2);
            }
        }

        // Allocate memory for the next line
        lines[nl] = malloc(max_line_len);
        if (lines[nl] == NULL) {
            fprintf(stderr, "Out of memory: line too long\n");
            exit(3);
        }

        // Read line
        if (fgets(lines[nl], max_line_len-1, fp) == NULL)
            // EOF reached
            break;

        // Remove EOL
        int j;
        for (j=strlen(lines[nl])-1;
             j>=0 && (lines[nl][j]=='\n' || lines[nl][j]=='\r');
             j--)
            ;
        lines[nl][j+1] = '\0';

        // Ignore the first line
        if (nl == 0 && consumed_first == 0) {
            nl--;
            consumed_first = 1;
        }
    }

    // Close the file
    fclose(fp);

    char *names[nl], *int_ips[nl], *ext_ips[nl];

    int n_fields;
    for (int i=0; i<nl; i++) {
        // Get the number of fields in the line
        n_fields = 1;
        for (int j=0; j<strlen(lines[i]); j++)
            if (lines[i][j] == ',')
                n_fields++;

        // Ignore the line if there are not 3 fields
        // Let the user know
        if (n_fields != 3) {
            char *reason = n_fields > 3 ? "too many" : "not enough";
            char *msg = "Ignoring line, %s fields: %s\n";
            fprintf(stderr, msg, reason, lines[i]);
            continue;
        }

        char *str = strdup(lines[i]);
        names[i] = strsep(&str, ",");
        int_ips[i] = strsep(&str, ",");
        ext_ips[i] = strsep(&str, ",");
    }

    // Run listbox with each hostname as a choice
    int choice = listbox(1, "Connect:", names, nl, "*");
    if (choice == -1)
        return 1;

    char *host;
    if (strcmp(int_ips[choice], "") == 0) {
        // There is no internal ip
        host = ext_ips[choice];
    } else if (strcmp(ext_ips[choice], "" ) == 0) {
        // There is no external ip
        host = int_ips[choice];
    } else {
        // There is both an internal and external ip
        puts("");
        // Run listbox with internal and external as choices
        char *choices[2] = {"Internal", "External"};
        char *values[2] = {int_ips[choice], ext_ips[choice]};
        int choice2 = listbox(0, "Host Location:", choices, 2, "*");
        host = values[choice2];
    }

    puts("");

    // Free memory
    for (; nl>=0; nl--)
        free(lines[nl]);
    free(lines);

    // Try to ssh to the chosen host
    attempt_ssh(host);

    return 0;
}

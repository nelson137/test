#include "lssh.c"

char *get_gateway(void) {
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


char *get_int_ip(char *host) {
    // Get current working directory
    int cwd_size = 100;
    char *cwd = malloc(sizeof(char*) * cwd_size);
    while (getcwd(cwd, cwd_size-1) == NULL) {
        cwd_size *= 2;
        cwd = realloc(cwd, cwd_size);
    }

    // Append connections.json.lock to cwd
    char conns_lock[cwd_size+22];
    sprintf(conns_lock, "%s/%s", cwd, "connections.json.lock");

    // Wait for the lock to go away if it exists
    do {
        if (access(conns_lock, R_OK) == -1)
            break;
        else
            sleep(1);
    } while(1);

    // Get the absolute path of connections.json
    char conns_fn[cwd_size+17];
    sprintf(conns_fn, "%s/%s", cwd, "connections.json");
    // Open connections.json
    FILE *fp = fopen(conns_fn, "r");

    // Read connections.json
    char *connections = NULL;
    size_t f_size;
    ssize_t f_bytes_read = getdelim(&connections, &f_size, '\0', fp);
    if (f_bytes_read == -1) {
        fprintf(stderr, "Could not read file %s\n", conns_fn);
        exit(1);
    }

    fclose(fp);

    // Create a string of the command to be executed
    char cmd_s[23+strlen(host)+sizeof(conns_fn)];
    sprintf(cmd_s, "jq -r '.%s.internal_ip' %s", host, conns_fn);

    // Execute the command
    FILE *cmd = popen(cmd_s, "r");
    if (cmd == NULL) {
        fprintf(stderr, "Failed to run jq\n");
        exit(1);
    }

    // Read jq output
    char *output = NULL;
    size_t o_size;
    ssize_t o_bytes_read = getdelim(&output, &o_size, '\0', cmd);
    if (o_bytes_read == -1) {
        fprintf(stderr, "Could not read output of jq command\n");
        exit(1);
    }

    pclose(cmd);

    return output;
}


void attempt_ssh_with_fix(char *host) {
    int ret;
    int pid = fork();
    if (pid < 0) {
        // fork() failed
        perror("fork failure");
        exit(1);
    } else if (pid == 0) {
        // Executes in child process
        execl("/usr/bin/ssh", "ssh", "-o", "ConnectTimeout=5", host, (char*)NULL);
        _exit(1);
    } else {
        // Executes in parent process
        // Wait for exec to complete
        wait(&ret);
    }

    if (WIFEXITED(ret)) {
        puts("child terminated normally");
        printf("exit status: %d\n", WEXITSTATUS(ret));
    } else if (WIFSIGNALED(ret)) {
        puts("child terminated by sig");
        printf("sig: %d\n", WTERMSIG(ret));
        if (WCOREDUMP(ret))
            puts("child produced core dump");
    } else if (WIFSTOPPED(ret)) {
        puts("child was stopped by sig");
        printf("sig: %d\n", WSTOPSIG(ret));
    }

    if (ret == 0)
        return;

    printf("\nError: could not ssh to host: %s\n", host);
    puts("Would you like to enter a new ip/domain name for this host?");
    char *choices[2] = {"Yes", "No"};
    int choice = listbox(0, NULL, choices, 2, "*");
    printf("chosen: %d\n", choice);
}

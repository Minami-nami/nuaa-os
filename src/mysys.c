#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define MAXCOMMANDLEN 256
#define MAXARGC 32

int ret = EXIT_SUCCESS;
void mysys(const char *Command) {
    char command[MAXCOMMANDLEN];
    char *argv[MAXARGC];
    int argc = 0;
    memset(argv, 0, sizeof(argv));
    strcpy(command, Command);
    argv[argc] = strtok(command, " ");
    while (argv[argc] != NULL) {
        argv[++argc] = strtok(NULL, " ");
    }

    pid_t subprocess = fork();
    int status;
    if (subprocess < 0) {
        perror("mysys");
        ret = EXIT_FAILURE;
        return;
    }
    else if (subprocess == 0) {
        int code = execvp(argv[0], argv);
        if (code == -1) {
            perror("mysys");
            ret = EXIT_FAILURE;
            return;
        }
    }

    wait(&status);
}

int main(int argc, char *argv[]) {
    printf("--------------------------------------------------\n");
    mysys("echo HELLO WORLD");
    printf("--------------------------------------------------\n");
    mysys("ls /");
    printf("--------------------------------------------------\n");
    return ret;
}
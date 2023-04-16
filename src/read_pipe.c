#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define MAXCOMMANDLEN 256
#define MAXARGC 32

void clean(int *pipe_fd) {
    if (pipe_fd[0] != -1) close(pipe_fd[0]);
    pipe_fd[0] = -1;
    if (pipe_fd[1] != -1) close(pipe_fd[1]);
    pipe_fd[1] = -1;
}

void read_pipe(const char *_command) {
    int fd[2];
    pipe(fd);
    pid_t pid = fork();
    if (pid == 0) {
        if (dup2(fd[1], STDOUT_FILENO) == -1) {
            perror("pipe");
            exit(EXIT_FAILURE);
        }
        char command[MAXCOMMANDLEN];
        char *argv[MAXARGC];
        int argc = 0;
        memset(argv, 0, sizeof(argv));
        strcpy(command, _command);
        argv[argc] = strtok(command, " ");
        while (argv[argc] != NULL) {
            argv[++argc] = strtok(NULL, " ");
        }
        clean(fd);
        int code = execvp(argv[0], argv);
        if (code == -1) {
            perror("pipe");
            exit(EXIT_FAILURE);
        }
    }
    else if (pid < 0) {
        perror("pipe");
        clean(fd);
        exit(EXIT_FAILURE);
    }
    else {
        int status;
        char buf[BUFSIZ];
        int len;
        close(fd[1]), fd[1] = -1;
        while ((len = read(fd[0], buf, BUFSIZ)) > 0) {
            write(STDOUT_FILENO, buf, len);
        }
        wait(&status);
        clean(fd);
    }
}

int main() {
    printf("--------------------------------------------------\n");
    read_pipe("echo HELLO WORLD");
    printf("--------------------------------------------------\n");
    read_pipe("ls /");
    printf("--------------------------------------------------\n");
    return 0;
}
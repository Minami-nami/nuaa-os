#include <stdio.h>
#include <sys/wait.h>
#include <unistd.h>
char buf[1024];
int  main() {
    int pipe_fd[2];
    pipe(pipe_fd);
    int pid = fork();
    if (pid == 0) {
        close(pipe_fd[0]);
        dup2(pipe_fd[1], STDOUT_FILENO);
        execlp("echo", "echo", "hello world", NULL);
    }
    else if (pid > 0) {
        close(pipe_fd[1]);
        dup2(pipe_fd[0], STDIN_FILENO);
        read(STDIN_FILENO, buf, sizeof(buf));
        puts(buf);
        wait(NULL);
    }
    return 0;
}
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>
int main() {
    int pipe_fd1[2];
    int pipe_fd2[2];
    pipe(pipe_fd1);
    pipe(pipe_fd2);
    int pid = fork();
    if (pid == 0) {
        close(pipe_fd1[0]);
        int sum = 0;
        for (int i = 1; i <= 50; ++i) {
            sum += i;
        }
        write(pipe_fd1[1], &sum, sizeof(sum));
        exit(0);
    }
    else if (pid > 0) {
        close(pipe_fd1[1]);
        int pid2 = fork();
        if (pid2 == 0) {
            close(pipe_fd2[0]);
            int sum = 0;
            for (int i = 51; i <= 100; ++i) {
                sum += i;
            }
            write(pipe_fd2[1], &sum, sizeof(sum));
            exit(0);
        }
        else if (pid2 > 0) {
            close(pipe_fd2[1]);
            int readed;
            int sum = 0;
            read(pipe_fd1[0], &readed, sizeof(readed));
            sum += readed;
            read(pipe_fd2[0], &readed, sizeof(readed));
            sum += readed;
            printf("%d\n", sum);
            return 0;
            wait(NULL);
            wait(NULL);
        }
    }
}
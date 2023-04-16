#include "src/cmd.h"
#include "utest.h"
#include <assert.h>
#include <fcntl.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#define BLUE "\e[0;34m"
#define L_BLUE "\e[1;34m"
#define GREEN "\e[0;32m"
#define L_GREEN "\e[1;32m"
#define PURPLE "\e[0;35m"
#define L_PURPLE "\e[1;35m"
#define CLOSE printf("\033[0m")  //关闭彩色字体

void read_and_cmp(char *path, char *expected) {
    int fd = open(path, O_RDONLY);

    char buff[1024];
    int  count  = read(fd, buff, sizeof(buff));
    buff[count] = 0;

    close(fd);
    assert(strcmp(buff, expected) == 0);
}

void test_exec_cmd() {
    struct cmd cmd = { 2, { "echo", "hello", NULL }, NULL, "test.out", 0 };
    unlink("test.out");

    pid_t pid = fork();
    if (pid == 0) {
        exec_cmd(&cmd);
    }
    wait(NULL);

    read_and_cmp("test.out", "hello\n");
}

// cat <test.in | sort | uniq | cat >test.out
void test_exec_pipe_cmd() {
    struct cmd cmdv[] = {
        { 1, { "cat", NULL }, "test.in", NULL, 0 },
        { 1, { "sort", NULL }, NULL, NULL, 0 },
        { 1, { "uniq", NULL }, NULL, NULL, 0 },
        { 1, { "cat", NULL }, NULL, "test.out", 0 },
    };
    unlink("test.out");

    pid_t pid = fork();
    if (pid == 0) {
        exec_pipe_cmd(4, cmdv);
    }
    wait(NULL);

    read_and_cmp("test.out", "1\n2\n3\n");
}

void utest_add_and_run() {
    UTEST_ADD(test_exec_cmd);
    UTEST_ADD(test_exec_pipe_cmd);
    utest_run();
}

int main(int argc, char *argv[]) {
    utest_parse_args(argc, argv, "-utest", utest_add_and_run);
    printf(GREEN "cmd unit test passed\n"), CLOSE;
    return 0;
}
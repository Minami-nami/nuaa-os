#include "src/parse.h"
#include "utest.h"
#include <assert.h>
#include <fcntl.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#define BLUE "\e[0;34m"
#define L_BLUE "\e[1;34m"
#define GREEN "\e[0;32m"
#define L_GREEN "\e[1;32m"
#define PURPLE "\e[0;35m"
#define L_PURPLE "\e[1;35m"
#define CLOSE printf("\033[0m")  //关闭彩色字体

// echo abc
void test_parse_cmd_1() {
    struct cmd cmd;
    char       line[] = "echo abc xyz";
    parse_cmd(line, &cmd);

    assert(cmd.argc == 3);
    assert(strcmp(cmd.argv[0], "echo") == 0);
    assert(strcmp(cmd.argv[1], "abc") == 0);
    assert(strcmp(cmd.argv[2], "xyz") == 0);
    assert(cmd.argv[3] == NULL);
}

// echo abc >log
void test_parse_cmd_2() {
    struct cmd cmd;
    char       line[] = "echo abc >log";
    parse_cmd(line, &cmd);

    assert(cmd.argc == 2);
    assert(strcmp(cmd.argv[0], "echo") == 0);
    assert(strcmp(cmd.argv[1], "abc") == 0);
    assert(strcmp(cmd.output, "log") == 0);
    assert(cmd.is_output_append == 0);
    assert(cmd.argv[2] == NULL);
}

// cat /etc/passwd | wc -l
void test_parse_pipe_cmd_1() {
    struct cmd cmdv[2];
    char       line[] = "cat /etc/passwd | wc -l";
    int        cmdc   = parse_pipe_cmd(line, cmdv);

    assert(cmdc == 2);
    assert(cmdv[0].argc == 2);
    assert(strcmp(cmdv[0].argv[0], "cat") == 0);
    assert(strcmp(cmdv[0].argv[1], "/etc/passwd") == 0);
    assert(cmdv[0].argv[2] == NULL);
    assert(cmdv[1].argc == 2);
    assert(strcmp(cmdv[1].argv[0], "wc") == 0);
    assert(strcmp(cmdv[1].argv[1], "-l") == 0);
    assert(cmdv[1].argv[2] == NULL);
}

// cat <input | sort | cat >output
void test_parse_pipe_cmd_2() {
    struct cmd cmdv[3];
    char       line[] = "cat <input | sort | cat >output";
    int        cmdc   = parse_pipe_cmd(line, cmdv);

    assert(cmdc == 3);
    assert(cmdv[0].argc == 1);
    assert(strcmp(cmdv[0].argv[0], "cat") == 0);
    assert(strcmp(cmdv[0].input, "input") == 0);
    assert(cmdv[0].argv[1] == NULL);
    assert(cmdv[1].argc == 1);
    assert(strcmp(cmdv[1].argv[0], "sort") == 0);
    assert(cmdv[1].argv[1] == NULL);
    assert(cmdv[2].argc == 1);
    assert(strcmp(cmdv[2].argv[0], "cat") == 0);
    assert(strcmp(cmdv[2].output, "output") == 0);
    assert(cmdv[2].is_output_append == 0);
    assert(cmdv[2].argv[1] == NULL);
}

void utest_add_and_run() {
    UTEST_ADD(test_parse_cmd_1);
    UTEST_ADD(test_parse_cmd_2);
    UTEST_ADD(test_parse_pipe_cmd_1);
    UTEST_ADD(test_parse_pipe_cmd_2);
    utest_run();
}

int main(int argc, char *argv[]) {
    utest_parse_args(argc, argv, "-utest", utest_add_and_run);
    printf(GREEN "parse unit test passed\n"), CLOSE;
    return 0;
}
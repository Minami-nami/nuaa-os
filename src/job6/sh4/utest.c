#include "utest.h"
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

#define BLUE "\e[0;34m"
#define L_BLUE "\e[1;34m"
#define GREEN "\e[0;32m"
#define L_GREEN "\e[1;32m"
#define PURPLE "\e[0;35m"
#define L_PURPLE "\e[1;35m"
#define RED "\e[0;31m"
#define L_RED "\e[1;31m"
#define WHITE "\e[1;37m"
#define GRAY "\e[0;37m"
#define CLOSE printf("\033[0m")  //关闭彩色字体

typedef struct {
    int   id;
    char *desc;
    void (*fp)();
    int ok;
} utest_t;

static int      utest_capacity;
static utest_t *utest_array;
static int      utest_count;
static int      utest_log;
static utest_t *curtest   = NULL;
static int      alarm_cnt = 3;

static void child_alarm(int signo) {
    kill(getppid(), SIGALRM);
    curtest->ok = 0;
    exit(EXIT_FAILURE);
}

static void parent_alarm(int signo) {
    static char errorBuf[256];
    snprintf(errorBuf, 256, "%s: Timeout after %d seconds\n", curtest->desc, alarm_cnt);
    write(STDERR_FILENO, errorBuf, strlen(errorBuf));
}

void utest_add(char *desc, void (*fp)()) {
    assert(utest_count <= utest_capacity);

    if (utest_capacity == 0) {
        utest_capacity = 2;
        utest_array    = malloc(utest_capacity * sizeof(utest_t));
    }

    if (utest_count == utest_capacity) {
        utest_capacity *= 2;
        utest_array = realloc(utest_array, utest_capacity * sizeof(utest_t));
    }

    utest_t *utest = utest_array + utest_count;
    utest->id      = utest_count;
    utest->desc    = strdup(desc);
    utest->fp      = fp;
    utest_count++;
}

void utest_parse_args(int argc, char *argv[], char *target_arg, void (*fp)()) {
    for (int i = 0; i < argc; i++) {
        char *arg = argv[i];
        if (strcmp(arg, target_arg) == 0) {
            fp();
            exit(EXIT_SUCCESS);
        }
    }
}

void utest_exec(utest_t *utest) {
    utest->fp();
}

void utest_run(void) {
    utest_log = open("utest.log", O_CREAT | O_WRONLY | O_TRUNC, 0644);
    dup2(utest_log, STDERR_FILENO);
    close(utest_log);
    signal(SIGALRM, parent_alarm);
    for (int i = 0; i < utest_count; i++) {
        curtest   = &utest_array[i];
        pid_t pid = fork();
        int   status;
        if (pid == 0) {
            alarm(3);
            signal(SIGALRM, child_alarm);
            utest_exec(&utest_array[i]);
            exit(0);
        }
        waitpid(pid, &status, 0);
        if (WIFEXITED(status) && WEXITSTATUS(status) == EXIT_SUCCESS) {
            utest_array[i].ok = 1;
        }
        else {
            utest_array[i].ok = 0;
        }
    }
    for (int i = 0; i < utest_count; i++) {
        utest_t *utest = &utest_array[i];
        printf(L_PURPLE "%0*d" WHITE ": ", 3, utest->id);
        printf(utest->ok ? L_GREEN : L_RED);
        printf("%-*s" WHITE " %s\n", 32, utest->desc, utest->ok ? "⭕" : "❌");
    }
    printf("\nThe following test failed:\n");
    for (int i = 0; i < utest_count; i++) {
        utest_t *utest = &utest_array[i];
        if (utest->ok == 0) printf(L_PURPLE "%0*d" WHITE ": " L_RED "%-*s\n" GRAY, 3, utest->id, 32, utest->desc);
    }
    printf("\nSee utest.log for details.\n" GRAY);
}
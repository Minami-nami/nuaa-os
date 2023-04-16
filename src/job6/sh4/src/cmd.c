#include "cmd.h"
#include "parse.h"
#include <assert.h>
#include <fcntl.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

int exec_cmd(struct cmd *cmd) {
    if (builtin_cmd(cmd)) {
        exit(EXIT_SUCCESS);
    }
    dump_cmd(cmd);
    execvp(cmd->argv[0], cmd->argv);
    perror("sh");
    exit(EXIT_FAILURE);
}

int builtin_cmd(struct cmd *cmd) {
    if (strcmp(cmd->argv[0], "cd") == 0) {
        if (cmd->argc == 1) {
            printf("\n");
            uid_t          uid      = getuid();
            struct passwd *pw       = getpwuid(uid);
            char          *user_dir = pw->pw_dir;
            if (chdir(user_dir) == -1) {
                perror("cd");
            }
        }
        else if (cmd->argc == 2) {
            if (chdir(cmd->argv[1]) == -1) {
                perror("cd");
            }
        }
        else if (cmd->argc > 2) {
            printf("cd: too many arguments\n");
        }
    }
    else if (strcmp(cmd->argv[0], "pwd") == 0) {
        int  MAXFILEPATH = 1024;
        char cwdbuf[MAXFILEPATH];
        if (getcwd(cwdbuf, MAXFILEPATH) == NULL) {
            perror("sh");
            return 1;
        }
        if (cmd->output != NULL) {
            int saved_stdout = dup(STDOUT_FILENO);
            int fd           = open(cmd->output, O_WRONLY | O_CREAT | (cmd->is_output_append ? O_APPEND : O_TRUNC), 0644);
            if (fd == -1) {
                perror("sh");
                return 1;
            }
            if (dup2(fd, STDOUT_FILENO) == -1) {
                perror("sh");
                return 1;
            }
            close(fd);
            printf("%s\n", cwdbuf);
            if (dup2(saved_stdout, STDOUT_FILENO) == -1) {
                perror("sh");
                return 1;
            }
            return 1;
        }
        printf("%s\n", cwdbuf);
    }
    else if (strcmp(cmd->argv[0], "exit") == 0) {
        if (cmd->argc == 1) {
            exit(0);
        }
        else if (cmd->argc == 2) {
            exit(atoi(cmd->argv[1]));
        }
        else if (cmd->argc > 2) {
            printf("exit: too many arguments\n");
        }
    }
    else
        return 0;
    return 1;
}

//递归执行管道命令

static void _exec_pipe_cmd(int cmdc, int index, struct cmd *cmdv) {
    int   pipefd[2];
    pid_t pid;
    int   state;
    if (pipe(pipefd) == -1) {
        perror("sh");
        return;
    }
    if ((pid = fork()) == -1) {
        perror("sh");
        return;
    }
    else if (pid == 0) {
        close(pipefd[0]);
        if (index != cmdc - 1 && dup2(pipefd[1], STDOUT_FILENO) == -1) {
            perror("sh");
            exit(EXIT_FAILURE);
        }
        close(pipefd[1]);
        exec_cmd(&cmdv[index]);
    }
    else {
        waitpid(pid, &state, 0);
        if (!(WIFEXITED(state) && WEXITSTATUS(state) == EXIT_SUCCESS)) {
            perror("sh");
            return;
        }
        close(pipefd[1]);
        if (dup2(pipefd[0], STDIN_FILENO) == -1) {
            perror("sh");
            return;
        }
        close(pipefd[0]);
        if (index < cmdc - 1) _exec_pipe_cmd(cmdc, index + 1, cmdv);
    }
}

void exec_pipe_cmd(int cmdc, struct cmd *cmdv) {
    if (cmdc == 1 && builtin_cmd(cmdv)) {
        return;
    }
    int save_in  = dup(STDIN_FILENO);
    int save_out = dup(STDOUT_FILENO);

    _exec_pipe_cmd(cmdc, 0, cmdv);
    if (dup2(save_in, STDIN_FILENO) == -1) {
        perror("sh");
        return;
    }
    if (dup2(save_out, STDOUT_FILENO) == -1) {
        perror("sh");
        return;
    }
    close(save_in);
    close(save_out);
}
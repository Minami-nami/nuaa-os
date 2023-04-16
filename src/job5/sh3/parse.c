#include "parse.h"
#include <fcntl.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

enum TOKEN_TYPE { TOKEN_TRUNCATED, TOKEN_APPEND, TOKEN_STR, TOKEN_START, TOKEN_INPUT, /*TOKEN_DOC*/ };

enum STATE_TYPE { STATE_FILE, STATE_TRUNC, STATE_APPEND, STATE_INPUT, /*STATE_DOC*/ };

#define MAXBUFSIZ 1024

char *strreplace(char *dest, char *src, const char *oldstr, const char *newstr, size_t len) {
    if (strcmp(oldstr, newstr) == 0) return src;
    char *needle;
    char *tmp;
    dest = src;

    while ((needle = strstr(dest, oldstr)) && (needle - dest <= len)) {
        tmp = (char *)malloc(strlen(dest) + (strlen(newstr) - strlen(oldstr)) + 1);

        strncpy(tmp, dest, needle - dest);

        tmp[needle - dest] = '\0';

        strcat(tmp, newstr);

        strcat(tmp, needle + strlen(oldstr));

        if (dest != src) free(dest);

        dest = strdup(tmp);

        free(tmp);
    }

    return dest;
}

int parse_cmd(char *line, struct cmd *cmd) {
    char *start           = line;
    char *ptr             = line;
    int   state           = STATE_FILE;
    int   token_type      = TOKEN_START;
    cmd->argc             = 0;
    cmd->input            = NULL;
    cmd->output           = NULL;
    cmd->is_output_append = 0;
    cmd->argv[0]          = NULL;
    while (*ptr != '\0') {
        token_type = TOKEN_START;
        while (*ptr != '\0') {
            switch (*ptr) {
            case ' ':
            case '\n':
            case '\t':
                *ptr++ = '\0';
                switch (token_type) {
                case TOKEN_START:
                    continue;
                default:
                    goto PARSER;
                }
            case '<':
                switch (token_type) {
                case TOKEN_START:
                    *ptr++     = '\0';
                    token_type = TOKEN_INPUT;
                    continue;
                case TOKEN_INPUT:
                case TOKEN_STR:
                case TOKEN_APPEND:
                case TOKEN_TRUNCATED:
                    goto PARSER;
                }
            case '>':
                switch (token_type) {
                case TOKEN_START:
                    *ptr++     = '\0';
                    token_type = TOKEN_TRUNCATED;
                    continue;
                case TOKEN_TRUNCATED:
                    *ptr++     = '\0';
                    token_type = TOKEN_APPEND;
                    continue;
                case TOKEN_STR:
                case TOKEN_INPUT:
                case TOKEN_APPEND:
                    goto PARSER;
                }
            default:
                switch (token_type) {
                case TOKEN_START:
                    start      = ptr;
                    token_type = TOKEN_STR;
                case TOKEN_STR:
                    ++ptr;
                    continue;
                case TOKEN_INPUT:
                case TOKEN_APPEND:
                case TOKEN_TRUNCATED:
                    goto PARSER;
                }
            }
        }
    PARSER:
        switch (token_type) {
        case TOKEN_APPEND:
            switch (state) {
            case STATE_FILE:
                state = STATE_APPEND;
                continue;
            default:
                write(STDERR_FILENO, "sh: parse error", 15);
                return 0;
            }
        case TOKEN_TRUNCATED:
            switch (state) {
            case STATE_FILE:
                state = STATE_TRUNC;
                continue;
            default:
                write(STDERR_FILENO, "sh: parse error", 15);
                return 0;
            }
        case TOKEN_INPUT:
            switch (state) {
            case STATE_FILE:
                state = STATE_INPUT;
                continue;
            default:
                write(STDERR_FILENO, "sh: parse error", 15);
                return 0;
            }
        case TOKEN_STR:
            switch (state) {
            case STATE_FILE:
                cmd->argv[cmd->argc++] = start;
                break;
            case STATE_APPEND:
                cmd->output           = start;
                cmd->is_output_append = 1;
                break;
            case STATE_TRUNC:
                cmd->output           = start;
                cmd->is_output_append = 0;
                break;
            case STATE_INPUT:
                cmd->input = start;
                break;
            }
            state = STATE_FILE;
            continue;
        }
    }
    cmd->argv[cmd->argc] = NULL;
    return 1;
}

void dump_cmd(struct cmd *cmd) {
    //根据cmd->input, cmd->output重定向标准输入输出
    if (cmd->input != NULL) {
        int fd = open(cmd->input, O_RDONLY);
        if (fd == -1) {
            perror("sh");
            return;
        }
        if (dup2(fd, STDIN_FILENO) == -1) {
            perror("sh");
            return;
        }
        close(fd);
    }
    if (cmd->output != NULL) {
        int fd = open(cmd->output, O_WRONLY | O_CREAT | (cmd->is_output_append ? O_APPEND : O_TRUNC), 0644);
        if (fd == -1) {
            perror("sh");
            return;
        }
        if (dup2(fd, STDOUT_FILENO) == -1) {
            perror("sh");
            return;
        }
        close(fd);
    }
}

int parse_pipe_cmd(char *line, struct cmd *cmdv) {
    // parse shell command
    static struct passwd *pw = NULL;
    char                 *token;
    char                 *saveptr;
    int                   i    = 0;
    int                   cmdc = 0;
    //cmdv->argc                 = 0;
    //cmdv->input                = NULL;
    //cmdv->output               = NULL;
   // cmdv->is_output_append     = 0;
    //cmdv->argv[0]              = NULL;
    if (pw == NULL) {
        uid_t uid = getuid();
        pw        = getpwuid(uid);
    }
    char *dest = strreplace(dest, line, "~", pw->pw_dir, MAXBUFSIZ);
    if (line != dest) {
        strcpy(line, dest);
        free(dest);
    }
    for (token = strtok_r(line, "|", &saveptr); token != NULL; token = strtok_r(NULL, "|", &saveptr)) {
        if (parse_cmd(token, cmdv + i) == 0) return 0;
        i++;
        cmdc++;
    }
    return cmdc;
}

#undef MAXBUFSIZ

void dump_pipe_cmd(int cmdc, struct cmd *cmdv) {
    for (int i = 0; i < cmdc; i++) {
        dump_cmd(cmdv + i);
    }
}

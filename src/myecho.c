#include <dirent.h>
#include <fcntl.h>
#include <linux/limits.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#define MAXINPUT 256
#define MAXINPUTLEN 256
#define MAXREDIRECT 256
#define MAXFILEPATH 256

enum TOKEN_TYPE { TOKEN_TRUNCATED, TOKEN_APPEND, TOKEN_STR, TOKEN_START };

enum STATE_TYPE { STATE_FILE, STATE_TRUNC, STATE_APPEND, STATE_STR };

enum DUP_TYPE { DUP_APPEND, DUP_TRUNC, DUP_INPUT };

const char *error_n  = "echo: parse error near '\\n'\n";
const char *error_rr = "echo: parse error near '>>'\n";
const char *error_r  = "echo: parse error near '>'\n";
char inputs[MAXINPUT][MAXINPUTLEN];
char redirect[MAXREDIRECT][MAXFILEPATH];
int type[MAXREDIRECT];
int lenInput = 0, lenRedirect = 0;
int state = STATE_STR;

void echo(const int len, const char output[MAXINPUT][MAXINPUTLEN]);

void echo(const int len, const char output[MAXINPUT][MAXINPUTLEN]) {
    for (int i = 0; i < len; ++i) {
        write(STDOUT_FILENO, output[i], strlen(output[i]));
        write(STDOUT_FILENO, " ", 1);
    }
    write(STDOUT_FILENO, "\n", 1);
    fflush(stdout);
}

void parse(const char *arg) {
    int len        = strlen(arg);
    int startidx   = 0;
    int token_type = TOKEN_START;
    for (int i = 0; i < len;) {
        token_type = TOKEN_START;
        startidx   = i;
        for (; i < len; ++i) {  // LEXER
            switch (arg[i]) {
            case '>': {
                switch (token_type) {
                case TOKEN_START: {
                    token_type = TOKEN_TRUNCATED;
                    continue;
                }
                case TOKEN_TRUNCATED: {
                    token_type = TOKEN_APPEND;
                    continue;
                }
                case TOKEN_APPEND:
                case TOKEN_STR: {
                    goto PARSER;
                }
                }
            }
            default: {
                switch (token_type) {
                case TOKEN_START:
                case TOKEN_STR: {
                    token_type = TOKEN_STR;
                    continue;
                }
                case TOKEN_APPEND:
                case TOKEN_TRUNCATED: {
                    goto PARSER;
                }
                }
            }
            }
        }

    PARSER:
        switch (token_type) {
        case TOKEN_APPEND: {
            switch (state) {
            case STATE_STR:
            case STATE_FILE: {
                state = STATE_APPEND;
                break;
            }
            default: {
                write(STDERR_FILENO, error_r, strlen(error_r));
                exit(EXIT_FAILURE);
            }
            }
            break;
        }
        case TOKEN_TRUNCATED: {
            switch (state) {
            case STATE_STR:
            case STATE_FILE: {
                state = STATE_TRUNC;
                break;
            }
            default: {
                write(STDERR_FILENO, error_rr, strlen(error_rr));
                exit(EXIT_FAILURE);
            }
            }
            break;
        }
        case TOKEN_STR: {
            switch (state) {
            case STATE_FILE:
                state = STATE_STR;
            case STATE_STR: {
                memcpy(inputs[lenInput++], arg + startidx, i - startidx);
                break;
            }
            case STATE_APPEND: {
                state = STATE_FILE;
                memcpy(redirect[lenRedirect], arg + startidx, i - startidx);
                type[lenRedirect++] = DUP_APPEND;
                break;
            }
            case STATE_TRUNC: {
                state = STATE_FILE;
                memcpy(redirect[lenRedirect], arg + startidx, i - startidx);
                type[lenRedirect++] = DUP_TRUNC;
                break;
            }
            }
            break;
        }
        }
    }
}

int main(int argc, char *argv[]) {
    char buffer[MAXNAMLEN];
    memset(buffer, 0, MAXNAMLEN);
    for (int i = 1; i < argc; ++i) {
        parse(argv[i]);
    }
    for (int i = 0; i < lenRedirect; ++i) {
        struct stat s;
        if (stat(redirect[i], &s) == -1) {
            perror("echo: ");
            continue;
        }
        if (S_ISDIR(s.st_mode)) {
            snprintf(buffer, MAXNAMLEN, "echo: is a directory: '%s'\n", redirect[i]);
            write(STDERR_FILENO, buffer, strlen(buffer));
            continue;
        }
        int new_out = dup(STDOUT_FILENO);
        if (new_out == -1) {
            perror("echo");
            return EXIT_FAILURE;
        }
        int code = open(redirect[i], (type[i] == DUP_APPEND ? O_APPEND : O_TRUNC) | O_WRONLY | O_CREAT);
        if (code == -1) {
            perror("echo");
            continue;
        }
        if (dup2(code, STDOUT_FILENO) == -1) {
            perror("echo");
            continue;
        }
        close(code);
        echo(lenInput, inputs);
        if (dup2(new_out, STDOUT_FILENO) == -1) {
            perror("echo");
            return EXIT_FAILURE;
        }
        close(new_out);
    }
    if (lenRedirect == 0) {
        echo(lenInput, inputs);
    }
    return EXIT_SUCCESS;
}
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
#define DOCLEN 32
#define TOKENLEN 32

enum TOKEN_TYPE { TOKEN_TRUNCATED, TOKEN_APPEND, TOKEN_STR, TOKEN_START, TOKEN_INPUT, TOKEN_DOC };

enum STATE_TYPE { STATE_FILE, STATE_TRUNC, STATE_APPEND, STATE_INPUT, STATE_DOC };

enum DUP_TYPE { DUP_APPEND, DUP_TRUNC, DUP_INPUT, DUP_NONE };

char redirect_in[MAXREDIRECT][MAXFILEPATH];
char redirect_out[MAXREDIRECT][MAXFILEPATH];
char errorBuf[MAXINPUT];
char tokenBuf[TOKENLEN];
char lineBuf[MAXINPUT];
char docs[BUFSIZ][DOCLEN];
int type_out[MAXREDIRECT];
int lenRedirectIn = 0, lenRedirectOut = 0, lenDoc = 0;
int state = STATE_FILE;

void cat();

void cat() {
    char buffer[BUFSIZ];
    int len = 0;
    while ((len = read(STDIN_FILENO, buffer, BUFSIZ)) > 0) {
        write(STDOUT_FILENO, buffer, len);
    }
    putchar('\n');
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
                case TOKEN_STR:
                case TOKEN_DOC:
                case TOKEN_INPUT:
                case TOKEN_APPEND: {
                    goto PARSER;
                }
                }
            }
            case '<': {
                switch (token_type) {
                case TOKEN_START: {
                    token_type = TOKEN_INPUT;
                    continue;
                }
                case TOKEN_INPUT: {
                    token_type = TOKEN_DOC;
                    continue;
                }
                case TOKEN_STR:
                case TOKEN_DOC:
                case TOKEN_APPEND:
                case TOKEN_TRUNCATED: {
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
                case TOKEN_DOC:
                case TOKEN_INPUT:
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
            case STATE_FILE: {
                state = STATE_APPEND;
                break;
            }
            default: {
                strncpy(tokenBuf, arg + startidx, i - startidx);
                snprintf(errorBuf, MAXINPUT, "cat: parse error near '%s'\n", tokenBuf);
                write(STDERR_FILENO, errorBuf, strlen(errorBuf));
                exit(EXIT_FAILURE);
            }
            }
            break;
        }
        case TOKEN_TRUNCATED: {
            switch (state) {
            case STATE_FILE: {
                state = STATE_TRUNC;
                break;
            }
            default: {
                strncpy(tokenBuf, arg + startidx, i - startidx);
                snprintf(errorBuf, MAXINPUT, "cat: parse error near '%s'\n", tokenBuf);
                write(STDERR_FILENO, errorBuf, strlen(errorBuf));
                exit(EXIT_FAILURE);
            }
            }
            break;
        }
        case TOKEN_INPUT: {
            switch (state) {
            case STATE_FILE: {
                state = STATE_INPUT;
                break;
            }
            default: {
                strncpy(tokenBuf, arg + startidx, i - startidx);
                snprintf(errorBuf, MAXINPUT, "cat: parse error near '%s'\n", tokenBuf);
                write(STDERR_FILENO, errorBuf, strlen(errorBuf));
                exit(EXIT_FAILURE);
            }
            }
            break;
        }
        case TOKEN_DOC: {
            switch (state) {
            case STATE_FILE: {
                state = STATE_DOC;
                break;
            }
            default: {
                strncpy(tokenBuf, arg + startidx, i - startidx);
                snprintf(errorBuf, MAXINPUT, "cat: parse error near '%s'\n", tokenBuf);
                write(STDERR_FILENO, errorBuf, strlen(errorBuf));
                exit(EXIT_FAILURE);
            }
            }
            break;
        }
        case TOKEN_STR: {
            struct stat s;
            switch (state) {
            case STATE_FILE: {
                strncpy(redirect_in[lenRedirectIn], arg + startidx, i - startidx);
                if (stat(redirect_in[lenRedirectIn], &s) == -1) {
                    perror("cat");
                    exit(EXIT_FAILURE);
                }
                else if (S_ISDIR(s.st_mode)) {
                    snprintf(errorBuf, MAXINPUT, "cat: '%s' is a directory\n", redirect_in[lenRedirectIn]);
                    write(STDERR_FILENO, errorBuf, strlen(errorBuf));
                    exit(EXIT_FAILURE);
                }
                type_out[lenRedirectIn++] = DUP_NONE;
                break;
            }
            case STATE_APPEND: {
                strncpy(redirect_out[lenRedirectOut], arg + startidx, i - startidx);
                stat(redirect_out[lenRedirectOut], &s);
                if (S_ISDIR(s.st_mode)) {
                    snprintf(errorBuf, MAXINPUT, "cat: '%s' is a directory\n", redirect_out[lenRedirectOut]);
                    write(STDERR_FILENO, errorBuf, strlen(errorBuf));
                    exit(EXIT_FAILURE);
                }
                type_out[lenRedirectOut++] = DUP_APPEND;
                break;
            }
            case STATE_TRUNC: {
                strncpy(redirect_out[lenRedirectOut], arg + startidx, i - startidx);
                stat(redirect_out[lenRedirectOut], &s);
                if (S_ISDIR(s.st_mode)) {
                    snprintf(errorBuf, MAXINPUT, "cat: '%s' is a directory\n", redirect_out[lenRedirectOut]);
                    write(STDERR_FILENO, errorBuf, strlen(errorBuf));
                    exit(EXIT_FAILURE);
                }
                type_out[lenRedirectOut++] = DUP_TRUNC;
                break;
            }
            case STATE_INPUT: {
                strncpy(redirect_in[lenRedirectIn], arg + startidx, i - startidx);
                if (stat(redirect_out[lenRedirectIn], &s) == -1) {
                    perror("cat");
                    exit(EXIT_FAILURE);
                }
                else if (S_ISDIR(s.st_mode)) {
                    snprintf(errorBuf, MAXINPUT, "cat: '%s' is a directory\n", redirect_in[lenRedirectIn]);
                    write(STDERR_FILENO, errorBuf, strlen(errorBuf));
                    exit(EXIT_FAILURE);
                }
                break;
            }
            case STATE_DOC: {
                strncpy(tokenBuf, arg + startidx, i - startidx);
                while (printf("heredoc> "), scanf("%s", lineBuf), strcmp(lineBuf, tokenBuf) != 0) {
                    strncat(docs[lenDoc], lineBuf, strlen(lineBuf));
                    strncat(docs[lenDoc], "\n", 1);
                }
                ++lenDoc;
                break;
            }
            }
            state = STATE_FILE;
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
    for (int i = 0; i < lenRedirectOut; ++i) {
        int new_out = dup(STDOUT_FILENO);
        if (new_out == -1) {
            perror("cat");
            return EXIT_FAILURE;
        }
        int code_out = open(redirect_out[i], (type_out[i] == DUP_APPEND ? O_APPEND : O_TRUNC) | O_WRONLY);
        if (code_out == -1 || dup2(code_out, STDOUT_FILENO) == -1) {
            perror("cat");
            return EXIT_FAILURE;
        }
        for (int j = 0; j < lenRedirectIn; ++j) {
            int new_in = dup(STDIN_FILENO);
            if (new_in == -1) {
                perror("cat");
                return EXIT_FAILURE;
            }
            int code_in = open(redirect_in[i], O_RDONLY);
            if (code_in == -1 || dup2(code_in, STDIN_FILENO) == -1) {
                perror("cat");
                return EXIT_FAILURE;
            }
            cat();
            if (dup2(new_in, STDIN_FILENO) == -1) {
                perror("cat");
                return EXIT_FAILURE;
            }
            close(code_in);
            close(new_in);
        }
        for (int j = 0; j < lenDoc; ++j) {
            write(STDOUT_FILENO, docs[j], strlen(docs[j]));
            putchar('\n');
            fflush(stdout);
        }
        if (lenRedirectIn == 0 && lenDoc == 0) {
            cat();
        }
        close(code_out);
        if (dup2(new_out, STDOUT_FILENO) == -1) {
            perror("cat");
            return EXIT_FAILURE;
        }
        close(new_out);
    }
    if (lenRedirectOut == 0) {
        for (int i = 0; i < lenRedirectIn; ++i) {
            int new_in = dup(STDIN_FILENO);
            if (new_in == -1) {
                perror("cat");
                return EXIT_FAILURE;
            }
            int code_in = open(redirect_in[i], O_RDONLY);
            if (code_in == -1 || dup2(code_in, STDIN_FILENO) == -1) {
                perror("cat");
                return EXIT_FAILURE;
            }
            cat();
            if (dup2(new_in, STDIN_FILENO) == -1) {
                perror("cat");
                return EXIT_FAILURE;
            }
            close(code_in);
            close(new_in);
        }
        for (int j = 0; j < lenDoc; ++j) {
            write(STDOUT_FILENO, docs[j], strlen(docs[j]));
            putchar('\n');
            fflush(stdout);
        }
    }

    return EXIT_SUCCESS;
}
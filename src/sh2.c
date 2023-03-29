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
#define MAXPATHLEN 256
#define MAXARGC 32
#define SYSTEMCALL 0
#define CD 1
#define PWD 2
#define EXIT 3
#define BLUE "\e[0;34m"
#define L_BLUE "\e[1;34m"
#define GREEN "\e[0;32m"
#define L_GREEN "\e[1;32m"
#define PURPLE "\e[0;35m"
#define L_PURPLE "\e[1;35m"
#define CLOSE printf("\033[0m")  //关闭彩色字体

#define MAXINPUT 256
#define MAXINPUTLEN 256
#define MAXREDIRECT 256
#define MAXFILEPATH 256
#define DOCLEN 32
#define TOKENLEN 32

enum TOKEN_TYPE { TOKEN_TRUNCATED, TOKEN_APPEND, TOKEN_STR, TOKEN_START, TOKEN_INPUT, TOKEN_DOC };

enum STATE_TYPE { STATE_FILE, STATE_TRUNC, STATE_APPEND, STATE_INPUT, STATE_DOC };

enum DUP_TYPE { DUP_APPEND, DUP_TRUNC, DUP_INPUT, DUP_DOC, DUP_NONE };

char redirect_in[MAXREDIRECT];
char redirect_out[MAXREDIRECT];
int out_type, in_type;
int isRin = -1, isRout = -1;
char tokenBuf[MAXREDIRECT];
char errorBuf[MAXINPUT];
char lineBuf[MAXINPUT];
char cwdbuf[MAXPATHLEN];
char docs[BUFSIZ];
int state = STATE_FILE;
int ret   = EXIT_SUCCESS;
int type  = SYSTEMCALL;
char command[MAXCOMMANDLEN];
char args[TOKENLEN][MAXARGC];
char *sys_argv[MAXARGC];
int sys_argc = 0;
uid_t uid;
struct passwd *pw;
char *user_dir;
char *user_name;
char *show_path;
char temp_file_name[] = "temp_file.XXXXXX";
char new_temp_file[]  = "temp_file.XXXXXX";

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

void _unlink() {
    if (isRin && in_type == DUP_DOC) {
        unlink(redirect_in);
    }
}

int parse(const char *arg) {
    int len      = strlen(arg);
    int startidx = 0;
    sys_argc     = 0;
    in_type = DUP_NONE, out_type = DUP_NONE;
    isRin = isRout = 0;
    int token_type = TOKEN_START;
    for (int i = 0; i < len;) {
        token_type = TOKEN_START;
        startidx   = i;
        memset(tokenBuf, 0, sizeof(tokenBuf));
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
            case ' ': {
                switch (token_type) {
                case TOKEN_START: {
                    ++startidx;
                    continue;
                }
                default:
                    goto PARSER;
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
                snprintf(errorBuf, MAXINPUT, "sh: parse error near '%s'\n", tokenBuf);
                write(STDERR_FILENO, errorBuf, strlen(errorBuf));
                return _unlink(), 0;
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
                snprintf(errorBuf, MAXINPUT, "sh: parse error near '%s'\n", tokenBuf);
                write(STDERR_FILENO, errorBuf, strlen(errorBuf));
                return _unlink(), 0;
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
                snprintf(errorBuf, MAXINPUT, "sh: parse error near '%s'\n", tokenBuf);
                write(STDERR_FILENO, errorBuf, strlen(errorBuf));
                return _unlink(), 0;
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
                snprintf(errorBuf, MAXINPUT, "sh: parse error near '%s'\n", tokenBuf);
                write(STDERR_FILENO, errorBuf, strlen(errorBuf));
                return _unlink(), 0;
            }
            }
            break;
        }
        case TOKEN_STR: {
            struct stat s;
            strncpy(tokenBuf, arg + startidx, i - startidx);
            char *dest = strreplace(dest, tokenBuf, "~", user_dir, MAXPATHLEN);
            strncpy(tokenBuf, dest, strlen(dest));
            switch (state) {
            case STATE_FILE: {
                strncpy(args[sys_argc++], tokenBuf, strlen(tokenBuf));
                break;
            }
            case STATE_APPEND: {
                stat(tokenBuf, &s);
                if (S_ISDIR(s.st_mode)) {
                    snprintf(errorBuf, MAXINPUT, "sh: '%s' is a directory\n", tokenBuf);
                    write(STDERR_FILENO, errorBuf, strlen(errorBuf));
                    return _unlink(), 0;
                }
                out_type = DUP_APPEND;
                isRout   = 1;
                strncpy(redirect_out, tokenBuf, strlen(tokenBuf));
                break;
            }
            case STATE_TRUNC: {
                stat(tokenBuf, &s);
                if (S_ISDIR(s.st_mode)) {
                    snprintf(errorBuf, MAXINPUT, "sh: '%s' is a directory\n", tokenBuf);
                    write(STDERR_FILENO, errorBuf, strlen(errorBuf));
                    return _unlink(), 0;
                }
                out_type = DUP_TRUNC;
                isRout   = 1;
                strncpy(redirect_out, tokenBuf, strlen(tokenBuf));
                break;
            }
            case STATE_INPUT: {
                if (stat(tokenBuf, &s) == -1) {
                    perror("sh");
                    return _unlink(), 0;
                }
                else if (S_ISDIR(s.st_mode)) {
                    snprintf(errorBuf, MAXINPUT, "sh: '%s' is a directory\n", redirect_in);
                    write(STDERR_FILENO, errorBuf, strlen(errorBuf));
                    return _unlink(), 0;
                }
                if (isRin && in_type == DUP_DOC) {
                    _unlink();
                }
                in_type = DUP_INPUT;
                isRin   = 1;
                strncpy(redirect_in, tokenBuf, strlen(tokenBuf));
                break;
            }
            case STATE_DOC: {
                memset(docs, 0, sizeof(docs));
                getchar();
                while (printf("heredoc> "), scanf("%[^\n]", lineBuf) != -1 && strcmp(lineBuf, tokenBuf) != 0) {
                    strncat(docs, lineBuf, strlen(lineBuf));
                    strncat(docs, "\n", 1);
                    getchar();
                }
                fflush(stdin);
                strncpy(temp_file_name, new_temp_file, strlen(new_temp_file));
                int code_temp = mkstemp(temp_file_name);
                if (code_temp == -1) {
                    perror("sh");
                    return _unlink(), 0;
                }
                write(code_temp, docs, strlen(docs));
                close(code_temp);
                if (isRin && in_type == DUP_DOC) {
                    _unlink();
                }
                in_type = DUP_DOC, isRin = 1;
                strncpy(redirect_in, temp_file_name, strlen(temp_file_name));
                break;
            }
            }
            state = STATE_FILE;
            break;
        }
        }
    }
    return 1;
}

void clean(int code_in, int code_out) {
    if (code_in != -1) close(code_in);
    if (code_out != -1) close(code_out);
}

void clean_built_in(int new_in, int new_out) {
    if (new_in != -1) {
        if (dup2(new_in, STDIN_FILENO) == -1) {
            perror("sh");
        }
        close(new_in);
    }
    if (new_out != -1) {
        if (dup2(new_out, STDOUT_FILENO) == -1) {
            perror("sh");
        }
        close(new_out);
    }
    _unlink();
}

int redirect() {
    int code_in = -1, code_out = -1;
    if (isRout) {
        if (out_type == DUP_APPEND) code_out = open(redirect_out, O_WRONLY | O_APPEND | O_CREAT, 0777);
        if (out_type == DUP_TRUNC) code_out = open(redirect_out, O_WRONLY | O_TRUNC | O_CREAT, 0777);
        if (code_out == -1) {
            perror("sh");
            return clean(code_in, code_out), 0;
        }
        if (dup2(code_out, STDOUT_FILENO) == -1) {
            perror("sh");
            return clean(code_in, code_out), 0;
        }
        close(code_out);
    }
    if (isRin) {
        code_in = open(redirect_in, O_RDONLY);
        if (code_in == -1) {
            perror("sh");
            return clean(code_in, code_out), 0;
        }
        if (dup2(code_in, STDIN_FILENO) == -1) {
            perror("sh");
            return clean(code_in, code_out), 0;
        }
        close(code_in);
    }
    return 1;
}

int redirect_built_in(int *in, int *out) {
    int new_in, new_out;
    new_out = dup(STDOUT_FILENO);
    if (new_out == -1) {
        perror("sh");
        return clean_built_in(new_in, new_out), 0;
    }
    new_in = dup(STDIN_FILENO);
    if (new_in == -1) {
        perror("sh");
        return clean_built_in(new_in, new_out), 0;
    }
    if (redirect() == 0) {
        return clean_built_in(new_in, new_out), 0;
    }
    *in = new_in, *out = new_out;
    return 1;
}

void built_in_call() {
    int new_in = -1, new_out = -1;
    redirect_built_in(&new_in, &new_out);
    switch (type) {
    case CD: {
        if (sys_argc == 1) {
            printf("\n");
        }
        else if (sys_argc > 2) {
            printf("cd: too many arguments\n");
            return clean_built_in(new_in, new_out);
        }
        int code = chdir(sys_argc == 1 ? user_dir : sys_argv[1]);
        if (code == -1) {
            perror("cd");
        }
        getcwd(cwdbuf, MAXPATHLEN);
        return clean_built_in(new_in, new_out);
    }
    case PWD: {
        if (sys_argc >= 2) {
            printf("pwd: too many arguments\n");
            return clean_built_in(new_in, new_out);
        }
        printf("%s\n", cwdbuf);
        return clean_built_in(new_in, new_out);
    }
    case EXIT: {
        if (sys_argc > 2) {
            printf("exit: too many arguments\n");
            return clean_built_in(new_in, new_out);
        }
        ret = (sys_argc == 2 ? atoi(sys_argv[1]) : 0);
        clean_built_in(new_in, new_out);
        exit(ret);
    }
    default:
        return clean_built_in(new_in, new_out);
    }
}

void mysys(const char *Command) {
    sys_argc = 0;
    type     = SYSTEMCALL;
    memset(args, 0, sizeof(args));
    memset(sys_argv, 0, sizeof(sys_argv));
    strcpy(command, Command);
    if (parse(command) == 0) return;  // PARSER
    sys_argv[0] = args[0];
    for (int i = 1; i < sys_argc; ++i) {
        sys_argv[i] = args[i];
    }
    if (strcmp(sys_argv[0], "cd") == 0) {
        type = CD;
    }
    else if (strcmp(sys_argv[0], "pwd") == 0) {
        type = PWD;
    }
    else if (strcmp(sys_argv[0], "exit") == 0) {
        type = EXIT;
    }
    if (type != SYSTEMCALL) {
        built_in_call();
        return;
    }
    pid_t subprocess = fork();
    int status;
    if (subprocess < 0) {
        perror("sh");
    }
    else if (subprocess == 0) {
        // printf("pid = %d, ppid = %d\n", getpid(), getppid());
        if (redirect() == 0) {
            exit(EXIT_FAILURE);
        }
        int code = execvp(sys_argv[0], sys_argv);
        if (code == -1) {
            // printf("EXECV ERROR FROM: %s\n", sys_argv[0]);
            perror("sh");
            exit(EXIT_FAILURE);
        }
    }
    else {
        // printf("pid = %d, ppid = %d\n", getpid(), getppid());
        wait(&status);
        _unlink();
    }
}

int main(int argc, char *argv[]) {
    uid       = getuid();
    pw        = getpwuid(uid);
    user_dir  = pw->pw_dir;
    user_name = pw->pw_name;
    getcwd(cwdbuf, MAXPATHLEN);
    while ((show_path = strreplace(show_path, cwdbuf, user_dir, "~", MAXPATHLEN)), printf(L_BLUE "%s " L_PURPLE "@%s " L_GREEN "> ", show_path, user_name), CLOSE, scanf("%[^\n]", command) != -1) {
        mysys(command);
        getchar();
    }
    return ret;
}
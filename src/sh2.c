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

enum DUP_TYPE { DUP_APPEND, DUP_TRUNC, DUP_INPUT, DUP_NONE };

char redirect_in[MAXREDIRECT];
char tokenBuf[MAXREDIRECT];
char errorBuf[MAXINPUT];
char lineBuf[MAXINPUT];
char docs[BUFSIZ];
int type_out[MAXREDIRECT];
int new_in = -1, new_out = -1;
int code_in = -1, code_out = -1, code_temp = -1;
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

void clean() {
    if (code_in != -1) close(code_in);
    code_in = -1;
    if (code_out != -1) close(code_out);
    code_out = -1;
    if (code_temp != -1) {
        unlink(temp_file_name);
        close(code_temp);
    }
    code_temp = -1;
    if (new_out != -1) {
        if (dup2(new_out, STDOUT_FILENO) == -1) perror("sh");
        close(new_out);
        new_out = -1;
    }
    if (new_in != -1) {
        if (dup2(new_in, STDIN_FILENO) == -1) perror("sh");
        close(new_in);
        new_in = -1;
    }
}
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

        dest = strdup(tmp);

        free(tmp);
    }

    return dest;
}

int parse(const char *arg) {
    int len        = strlen(arg);
    int startidx   = 0;
    sys_argc       = 0;
    int token_type = TOKEN_START;
    code_in = code_out = -1;
    new_out            = dup(STDOUT_FILENO);
    if (new_out == -1) {
        perror("sh");
        return clean(), 0;
    }
    new_in = dup(STDIN_FILENO);
    if (new_in == -1) {
        perror("sh");
        return clean(), 0;
    }
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
                return clean(), 0;
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
                return clean(), 0;
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
                return clean(), 0;
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
                return clean(), 0;
            }
            }
            break;
        }
        case TOKEN_STR: {
            struct stat s;
            strncpy(tokenBuf, arg + startidx, i - startidx);
            switch (state) {
            case STATE_FILE: {
                char *dest = strreplace(dest, tokenBuf, "~", user_dir, MAXPATHLEN);
                strncpy(args[sys_argc++], dest, strlen(dest));
                break;
            }
            case STATE_APPEND: {
                stat(tokenBuf, &s);
                if (S_ISDIR(s.st_mode)) {
                    snprintf(errorBuf, MAXINPUT, "sh: '%s' is a directory\n", tokenBuf);
                    write(STDERR_FILENO, errorBuf, strlen(errorBuf));
                    return clean(), 0;
                }
                code_out = open(tokenBuf, O_WRONLY | O_APPEND | O_APPEND);
                if (code_out == -1) {
                    perror("sh");
                    return clean(), 0;
                }
                if (dup2(code_out, STDOUT_FILENO) == -1) {
                    perror("sh");
                    return clean(), 0;
                }
                close(code_out);
                break;
            }
            case STATE_TRUNC: {
                stat(tokenBuf, &s);
                if (S_ISDIR(s.st_mode)) {
                    snprintf(errorBuf, MAXINPUT, "sh: '%s' is a directory\n", tokenBuf);
                    write(STDERR_FILENO, errorBuf, strlen(errorBuf));
                    return clean(), 0;
                }
                code_out = open(tokenBuf, O_WRONLY | O_TRUNC | O_CREAT, 0777);
                if (code_out == -1) {
                    perror("sh");
                    return clean(), 0;
                }
                if (dup2(code_out, STDOUT_FILENO) == -1) {
                    perror("sh");
                    return clean(), 0;
                }
                close(code_out);
                break;
            }
            case STATE_INPUT: {
                if (stat(tokenBuf, &s) == -1) {
                    perror("sh");
                    return clean(), 0;
                }
                else if (S_ISDIR(s.st_mode)) {
                    snprintf(errorBuf, MAXINPUT, "sh: '%s' is a directory\n", redirect_in);
                    write(STDERR_FILENO, errorBuf, strlen(errorBuf));
                    return clean(), 0;
                }
                code_in = open(tokenBuf, O_RDONLY);
                if (code_in == -1) {
                    perror("sh");
                    return clean(), 0;
                }
                if (dup2(code_in, STDIN_FILENO) == -1) {
                    perror("sh");
                    return clean(), 0;
                }
                close(code_in);
                break;
            }
            case STATE_DOC: {
                char t;
                memset(docs, 0, sizeof(docs));
                while (printf("heredoc> "), scanf("%[^\n]%c", lineBuf, &t), strcmp(lineBuf, tokenBuf) != 0) {
                    strncat(docs, lineBuf, strlen(lineBuf));
                    strncat(docs, "\n", 1);
                }
                fflush(stdin);
                if (code_temp != -1) close(code_temp);
                strncpy(temp_file_name, new_temp_file, strlen(new_temp_file));
                code_temp = mkstemp(temp_file_name);
                if (code_temp == -1) {
                    perror("sh");
                    return clean(), 0;
                }
                write(code_temp, docs, strlen(docs));
                close(code_temp);
                code_temp = open(temp_file_name, O_RDONLY);
                if (code_temp == -1) {
                    return clean(), 0;
                }
                if (dup2(code_temp, STDIN_FILENO) == -1) {
                    perror("sh");
                    return clean(), 0;
                }
                close(code_temp);
                unlink(temp_file_name);
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

void mysys(const char *Command) {
    sys_argc = 0;
    type     = SYSTEMCALL;
    memset(args, 0, sizeof(args));
    memset(sys_argv, 0, sizeof(sys_argv));
    strcpy(command, Command);
    if (parse(command) == 0) return clean();  // PARSER
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
    if (type != SYSTEMCALL) return;
    pid_t subprocess = fork();
    int status;
    if (subprocess < 0) {
        perror("sh");
    }
    else if (subprocess == 0) {
        //printf("pid = %d, ppid = %d\n", getpid(), getppid());
        int code = execvp(sys_argv[0], sys_argv);
        if (code == -1) {
            //printf("EXECV ERROR FROM: %s\n", sys_argv[0]);
            perror("sh");
            exit(EXIT_FAILURE);
        }
    }
    else {
        //printf("pid = %d, ppid = %d\n", getpid(), getppid());
        wait(&status);
        clean();
    }
}

int main(int argc, char *argv[]) {
    char cwdbuf[MAXPATHLEN];
    char delim;
    uid       = getuid();
    pw        = getpwuid(uid);
    user_dir  = pw->pw_dir;
    user_name = pw->pw_name;
    getcwd(cwdbuf, MAXPATHLEN);
    while ((show_path = strreplace(show_path, cwdbuf, user_dir, "~", MAXPATHLEN)), printf(L_BLUE "%s " L_PURPLE "@%s " L_GREEN "> ", show_path, user_name), CLOSE, scanf("%[^\n]%c", command, &delim)) {
        mysys(command);
        switch (type) {
        case CD: {
            if (sys_argc == 1) {
                printf("\n");
            }
            else if (sys_argc > 2) {
                printf("cd: too many arguments\n");
                break;
            }
            int code = chdir(sys_argc == 1 ? user_dir : sys_argv[1]);
            if (code == -1) {
                perror("cd");
            }
            getcwd(cwdbuf, MAXPATHLEN);
            break;
        }
        case PWD: {
            if (sys_argc >= 2) {
                printf("pwd: too many arguments\n");
                break;
            }
            char *dest_path;
            printf("%s\n", cwdbuf);
            break;
        }
        case EXIT: {
            if (sys_argc > 2) {
                printf("exit: too many arguments\n");
                break;
            }
            ret = (sys_argc == 2 ? atoi(sys_argv[1]) : 0);
            return ret;
        }
        default:
            break;
        }
    }
    return ret;
}
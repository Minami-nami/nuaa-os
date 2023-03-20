#include <errno.h>
#include <fcntl.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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

int            ret  = EXIT_SUCCESS;
int            type = SYSTEMCALL;
char           command[MAXCOMMANDLEN];
char          *sys_argv[MAXARGC];
int            sys_argc = 0;
uid_t          uid;
struct passwd *pw;
char          *user_dir;
char          *user_name;
char          *show_path;

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

void mysys(const char *Command) {
    sys_argc = 0;
    type     = SYSTEMCALL;
    memset(sys_argv, 0, sizeof(sys_argv));
    strcpy(command, Command);
    sys_argv[sys_argc] = strtok(command, " ");
    while (1) {
        sys_argv[++sys_argc] = strtok(NULL, " ");
        if (sys_argv[sys_argc] == NULL) break;
        char *dest;
        sys_argv[sys_argc] = strreplace(dest, sys_argv[sys_argc], "~", user_dir, MAXPATHLEN);
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
    int   status;
    if (subprocess < 0) {
        perror("mysys");
        ret = EXIT_FAILURE;
        return;
    }
    else if (subprocess == 0) {
        int code = execvp(sys_argv[0], sys_argv);
        if (code == -1) {
            perror("mysys");
            ret = EXIT_FAILURE;
            return;
        }
    }

    wait(&status);
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
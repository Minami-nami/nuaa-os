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

int   ret  = EXIT_SUCCESS;
int   type = SYSTEMCALL;
char  command[MAXCOMMANDLEN];
char *sys_argv[MAXARGC];
int   sys_argc = 0;

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
    while (sys_argv[sys_argc] != NULL) {
        sys_argv[++sys_argc] = strtok(NULL, " ");
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
    char           command[MAXCOMMANDLEN];
    char           cwdbuf[MAXPATHLEN];
    char           delim;
    uid_t          uid      = getuid();
    struct passwd *pw       = getpwuid(uid);
    const char    *user_dir = pw->pw_dir;
    while (printf("> "), scanf("%[^\n]%c", command, &delim)) {
        mysys(command);
        switch (type) {
        case CD: {
            if (sys_argc == 1) {
                printf("\n");
                break;
            }
            if (sys_argc > 2) {
                printf("cd: too many arguments\n");
                break;
            }
            char *dest_path;
            dest_path = strreplace(dest_path, sys_argv[1], "~", user_dir, MAXPATHLEN);
            int code  = chdir(dest_path);
            if (code == -1) {
                perror("cd");
            }
            break;
        }
        case PWD: {
            if (sys_argc >= 2) {
                printf("pwd: too many arguments\n");
                break;
            }
            char *dest_path;
            getcwd(cwdbuf, MAXPATHLEN);
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
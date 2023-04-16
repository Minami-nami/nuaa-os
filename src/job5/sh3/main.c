#include "cmd.h"
#include "parse.h"
#include <assert.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#define BLUE "\e[0;34m"
#define L_BLUE "\e[1;34m"
#define GREEN "\e[0;32m"
#define L_GREEN "\e[1;32m"
#define PURPLE "\e[0;35m"
#define L_PURPLE "\e[1;35m"
#define CLOSE printf("\033[0m")  //关闭彩色字体
#define MAXBUFSIZ 1024
#define MAXPIPE 10

void read_line(char *line, int size) {
    static char    buffer[MAXBUFSIZ];
    static char    cwdbuf[MAXBUFSIZ];
    static char   *show_path = NULL;
    static char   *user_dir  = NULL;
    static char   *user_name = NULL;
    struct passwd *pw        = NULL;
    if (pw == NULL) {
        uid_t uid = getuid();
        pw        = getpwuid(uid);
        user_dir  = pw->pw_dir;
        user_name = pw->pw_name;
    }
    getcwd(cwdbuf, MAXBUFSIZ);
    show_path = strreplace(show_path, cwdbuf, user_dir, "~", MAXBUFSIZ);
    printf(L_BLUE "%s " L_PURPLE "@%s " L_GREEN "> ", show_path, user_name), CLOSE;

    int readed = scanf("%[^\n]", buffer);
    if (readed == -1 || readed == 0)
        memset(line, 0, size);
    else {
        strcpy(line, buffer);
        getchar();
    }
}

int main() {
    while (1) {
        static char       line[MAXBUFSIZ];
        static struct cmd cmd[MAXPIPE];
        read_line(line, MAXBUFSIZ);
        size_t size = parse_pipe_cmd(line, (struct cmd *)cmd);
        exec_pipe_cmd(size, cmd);
    }
    return 0;
}

#include <dirent.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

int  ret = EXIT_SUCCESS;
void cat(const char *path);

void cat(const char *path) {
    struct stat _s;
    if (stat(path, &_s) == -1) {
        printf("cat: %s: No such file or directory\n", path);
        ret = EXIT_FAILURE;
        return;
    }
    if (S_ISDIR(_s.st_mode)) {
        printf("cat: %s: Is a directory\n", path);
        ret = EXIT_FAILURE;
        return;
    }
    int  code = open(path, O_RDONLY);
    char buffer[BUFSIZ];
    int  len = 0;
    while ((len = read(code, buffer, BUFSIZ)) > 0) {
        write(STDOUT_FILENO, buffer, len);
    }
    putchar('\n');
}

int main(int argc, char *argv[]) {
    for (int i = 1; i < argc; ++i) {
        cat(argv[i]);
    }
    return ret;
}
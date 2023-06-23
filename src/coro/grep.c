#include "coro.h"
#include <dirent.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PURPLE "\e[0;35m"
#define L_PURPLE "\e[1;35m"
#define RED "\e[0;31m"
#define L_RED "\e[1;31m"
#define BLUE "\e[0;34m"
#define L_BLUE "\e[1;34m"
#define CLOSE printf("\033[0m")

char   *root_dir   = NULL;
char   *string     = NULL;
coro_t *co_consume = NULL;
coro_t *co_produce = NULL;

#define PATHLEN 256

void find_dir(char *root) {
    DIR           *pdir = opendir(root);
    struct dirent *entry;
    char           path[PATHLEN];
    if (pdir == NULL) return;
    while ((entry = readdir(pdir))) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) continue;
        if (entry->d_type == DT_DIR) {
            strcpy(path, root);
            strcat(path, "/");
            strcat(path, entry->d_name);
            find_dir(path);
        }
        else if (entry->d_type == DT_REG) {
            strcpy(path, root);
            strcat(path, "/");
            strcat(path, entry->d_name);
            coro_yield((unsigned long long)path);
        }
    }
}

void fs_walk() {
    find_dir(root_dir);
    coro_yield(0);
}

void grep() {
    char *path;
    char *line = NULL;
    while (1) {
        path = (char *)coro_resume(co_produce);
        if (path == NULL) break;
        FILE *fp = fopen(path, "r");
        if (fp == NULL) continue;
        size_t len = 0;
        size_t nread;
        while (!(feof(fp)) && (nread = getline(&line, &len, fp)) > 0) {
            char  *p   = strstr(line, string);
            size_t len = strlen(string);
            if (p != NULL) {
                char *start = line;
                printf(PURPLE "%s" L_BLUE ":", path);
                CLOSE;
                for (; start < p; start++) {
                    printf("%c", *start);
                }
                printf(L_RED);
                for (size_t i = 0; i < len; ++i, ++start) {
                    printf("%c", *start);
                }
                CLOSE;
                printf("%s", start);
            }
            else
                continue;
        }
        fclose(fp);
    }
    free(line);
    exit(0);
}

void usage() {
    puts("grep -r string dir");
}

int main(int argc, char *argv[]) {
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-r") == 0) {
            if (i + 1 < argc)
                string = argv[++i];
            else
                return usage(), 1;
        }
        else {
            root_dir = argv[i];
        }
    }
    if (root_dir == NULL || string == NULL) return usage(), 1;
    co_produce = coro_new(fs_walk);
    co_consume = coro_new(grep);
    coro_boot(co_consume);
    return 0;
}
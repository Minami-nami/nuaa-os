#include "coro.h"
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char   *root_dir   = NULL;
char   *file_name  = NULL;
coro_t *co_consume = NULL;
coro_t *co_produce = NULL;

void usage() {
    puts("find -name file-name dir");
}

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

void find() {
    char *path;
    while (1) {
        path = (char *)coro_resume(co_produce);
        if (path == NULL) break;
        int lenP = strlen(path), lenF = strlen(file_name), flag = 0;
        if (lenP < lenF) continue;
        int dif = lenP - lenF;
        for (int i = lenF - 1; i >= 0; i--) {
            if (path[i + dif] != file_name[i]) {
                flag = 1;
                break;
            }
        }
        if (flag || path[dif - 1] != '/') continue;
        puts(path);
    }
    exit(0);
}

int main(int argc, char *argv[]) {
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-name") == 0) {
            if (i + 1 < argc)
                file_name = argv[++i];
            else
                return usage(), 1;
        }
        else {
            root_dir = argv[i];
        }
    }
    if (root_dir == NULL || file_name == NULL) return usage(), 1;
    co_produce = coro_new(fs_walk);
    co_consume = coro_new(find);
    coro_boot(co_consume);
    return 0;
}

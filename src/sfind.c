#include <dirent.h>
#include <libgen.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#define LINEBUF 256
#define MAXPATHLEN 256

void find_file(char *path, char *target) {
    FILE *file = fopen(path, "r");

    char line[LINEBUF];
    while (fgets(line, sizeof(line), file)) {
        if (strstr(line, target)) printf("%s: %s", path, line);
    }

    fclose(file);
}

void find_dir(char *path, char *target) {
    DIR           *dir = opendir(path);
    struct dirent *entry;
    while ((entry = readdir(dir))) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) continue;

        if (entry->d_type != DT_DIR && entry->d_type != DT_REG) continue;

        char *c_dir = malloc(MAXPATHLEN * sizeof(char));
        strcpy(c_dir, path);
        strcat(c_dir, "/");
        strcat(c_dir, entry->d_name);

        if (entry->d_type == DT_DIR) find_dir(c_dir, target);

        if (entry->d_type == DT_REG) find_file(c_dir, target);

        free(c_dir);
    }
    closedir(dir);
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        puts("Usage: sfind file string");
        return 0;
    }

    char *path   = argv[1];
    char *string = argv[2];

    struct stat info;
#ifdef DEBUG
    char Root_Dir[MAXPATHLEN];
    printf("path: %s\n", getcwd(Root_Dir, MAXPATHLEN));
#endif
    if ((stat(path, &info)) == -1) {
        perror("stat");
        return 1;
    }

    char *c_dir = strdup(path), *dir;
    chdir((dir = dirname(c_dir)));

    char *c_name = strdup(path), *name;
    name         = basename(c_name);

    if (S_ISDIR(info.st_mode))
        find_dir(name, string);
    else
        find_file(name, string);

    free(c_dir);
    free(c_name);
    return 0;
}

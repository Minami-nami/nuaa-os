#include <dirent.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define MAXPATHLEN 256
#define MODE (S_IRWXG | S_IRWXO | S_IRWXU)
#define DT_DIR 4
#define DT_REG 8

void copy_file_file(const char* srcpath, const char* destpath);
void copy_file_folder(const char* srcpath, const char* destpath);
void copy_folder_folder(char* srcpath, const char* destpath);
void err_fail_to_open(const char* path);
void err_fail_to_stat(const char* path);
void err_fail_to_make(const char* path);
char *GetFileName(char* path);
int is_same_dir(const char *src_path, const char *dst_path);

int is_same_dir(const char *src_path, const char *dst_path) {
    struct stat src_stat, dst_stat;
    if (stat(src_path, &src_stat) < 0 || stat(dst_path, &dst_stat) < 0) {
        return 0;
    }
    return (src_stat.st_dev == dst_stat.st_dev) && (src_stat.st_ino == dst_stat.st_ino);
}

void copy_file_file(const char* srcpath, const char* destpath) {
    struct stat src, dst;
    if (stat(srcpath, &src) != -1 && stat(destpath, &dst) != -1 && src.st_ino == dst.st_ino) {
        printf("cp: '%s' and '%s' are the same file\n", srcpath, destpath);
        return;
    }
    int code_src, code_dst;
    if ((code_src = open(srcpath, O_RDONLY)) == -1) {
        err_fail_to_open(srcpath);
        return;
    }
    if ((code_dst = open(destpath, O_WRONLY | O_CREAT | O_TRUNC, 0644)) == -1) {
        err_fail_to_open(destpath);
        return;
    }
    char buffer[BUFSIZ];
    int len = 0;
    while ((len = read(code_src, buffer, BUFSIZ)) > 0) {
        write(code_dst, buffer, len);
    }
    close(code_src);
    close(code_dst);
}

void copy_file_folder(const char* srcpath, const char* destpath) {
    char pathbuffer[MAXPATHLEN];
    snprintf(pathbuffer, MAXPATHLEN, "%s/%s", destpath, GetFileName(srcpath));
    copy_file_file(srcpath, pathbuffer);
}

void copy_folder_folder(char* srcpath, const char* destpath) {
    if (is_same_dir(srcpath, destpath)) {
        printf("cp: cannot copy a directory, '%s', into itself, '%s/%s'", srcpath, destpath, GetFileName(srcpath));
        return;
    }
    DIR* dir = NULL;
    struct stat dst_stat, src_stat;
    int exist_dst = (stat(destpath, &dst_stat) == 0);
    int exist_src = (stat(srcpath, &src_stat) == 0);
    int is_dir = S_ISDIR(dst_stat.st_mode);
    if (exist_dst && !is_dir) {
        printf("cp: cannot overwrite non-directory '%s' with directory '%s'\n", destpath, srcpath);
        return;
    }
    if (!exist_dst && mkdir(destpath, 0755) == -1) {
        err_fail_to_make(destpath);
        return;
    }
    char buffer[MAXPATHLEN];
    snprintf(buffer, MAXPATHLEN, "%s/%s", destpath, GetFileName(srcpath));
    if (mkdir(buffer, src_stat.st_mode & 0777) == -1 && access(buffer, F_OK) == -1) {
        err_fail_to_make(buffer);
        return;
    }
    struct dirent* dirent = NULL;
    if ((dir = opendir(srcpath)) == NULL) {
        err_fail_to_open(srcpath);
        return;
    }
    while (dirent = readdir(dir)) {
        if (strcmp(dirent->d_name, ".") == 0 || strcmp(dirent->d_name, "..") == 0)
            continue;
        char buffer_src[MAXPATHLEN];
        snprintf(buffer_src, MAXPATHLEN, "%s/%s", srcpath, dirent->d_name);
        if (dirent->d_type == DT_DIR) {
            copy_folder_folder(buffer_src, buffer);
        }
        else if (dirent->d_type == DT_REG){
            copy_file_folder(buffer_src, buffer);
        }
    }
    closedir(dir);
}

void err_fail_to_open(const char* path) {
    printf("cp: cannot open '%s'\n", path);
}

void err_fail_to_stat(const char* path) {
    printf("cp: cannot stat '%s': No such file or directory\n", path);
}

void err_fail_to_make(const char* path) {
    printf("cp: cannot make directory '%s'\n", path);
}

char *GetFileName(char* path) {
    int len = strlen(path);
    if (path[len - 1] == '/')
        path[len - 1] = '\0';
    char* ret = strrchr(path, '/');
    return ret == NULL? path: ret + 1;
}

int main(int argc, char *argv[]) {
    if (argc <= 1) { // cp -r
        puts("cp: missing file operand");
        return 0;
    }
    char* dst = argv[argc - 1];
    int recursive = (strcmp(argv[1], "-r") == 0);
    int len = argc - recursive - 2;
    struct stat srcfiles[len], dstfile;
    unsigned long ino[argc];
    memset(ino, 0, sizeof(ino));
    int exist_dst = (stat(dst, &dstfile) == 0);
    int is_dst_dir = S_ISDIR(dstfile.st_mode);
    unsigned long dst_inode = dstfile.st_ino;
    if (len < 1) { // cp -r src
        printf("cp: missing destination file operand after '%s'\n", argv[recursive + 1]);
        return 0;
    }
    if (!is_dst_dir && len > 1) {
        printf("cp: target '%s' is not a directory\n", dst);
        return 0;
    }
    for (int i = recursive + 1; i < argc - 1; ++i) {
        char* src = argv[i];
        int index = i - recursive - 1;
        struct stat* srcfile = srcfiles + index;
        int exist_src = (stat(src, srcfile) == 0);
        int is_src_dir = S_ISDIR(srcfile->st_mode);
        int flag = 0;
        if (!exist_src) {
            err_fail_to_stat(src);
            continue;
        }
        if (!recursive && is_src_dir) {
            printf("cp: -r not specified; omitting directory '%s'\n", src);
            continue;
        }
        ino[i] = srcfile->st_ino;
        for (int j = 0; j < i; ++j) {
            if (ino[i] == ino[j]) {
                flag = 1;
                break;
            }
        }
        if (flag == 1) {
            printf("cp: warning: source ");
            printf(is_src_dir? "directory ": "file" );
            printf("'%s' specified more than once\n", src);
            continue;
        }
        if (!is_src_dir) {
            if (is_dst_dir) {
                copy_file_folder(src, dst);
            }
            else {
                copy_file_file(src, dst);
            }
        }
        else {
            copy_folder_folder(src, dst);
        }
    }
}
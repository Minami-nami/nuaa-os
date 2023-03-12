#include <unistd.h>
#include <sys/stat.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <dirent.h>

#define READ_BUFFER_SIZE 32

void cp_dir_to_dir(const char* src_path, const char* dst_path);
void cp_file_to_dir(const char* src_path, const char* dst_path);
void cp_file_to_file(const char* src_path, const char* dst_path);
void err_fail_to_open(const char* file_path);

void cp_dir_to_dir(const char* src_path, const char* dst_path) {
    struct stat st_src, st_dst;
    if (stat(src_path, &st_src) == -1) {
        err_fail_to_open(src_path);
        return;
    }
    if (stat(dst_path, &st_dst) == -1) {
        err_fail_to_open(dst_path);
        return;
    }
}

void cp_file_to_dir(const char* src_path, const char* dst_path) {

}

void cp_file_to_file(const char* src_path, const char* dst_path) {
    int src, dst;
    if ((src = open(src_path, O_RDONLY)) == -1) {
        err_fail_to_open(src);
        return;
    }
    if ((dst = open(dst_path, O_WRONLY | O_CREAT | O_TRUNC)) == -1) {
        err_fail_to_open(dst);
        return;
    }
    char buffer[READ_BUFFER_SIZE];
    while (read(src, buffer, sizeof(buffer)) > 0) {
        write(dst, buffer, READ_BUFFER_SIZE);
    }
    close(src_path);
    close(dst_path);
}

void err_fail_to_open(const char* file_path) {
    printf("cp: cannot stat \'%s\': No such file or directory\n", file_path);
}

void cp(const char* src_path, const char* dst_path) {
    struct stat st_src, st_dst;
    if (stat(src_path, &st_src) == -1) {
        err_fail_to_open(src_path);
        return;
    }
    if (stat(dst_path, &st_dst) == -1) {// TODO
        err_fail_to_open(dst_path);
        return;
    }
    if (S_ISDIR(st_src.st_mode)) {
        cp
        return;
    }
    char buffer[READ_BUFFER_SIZE];
    int code = open(file_path, O_RDONLY);
    while (read(code, buffer, sizeof(buffer) - 1) > 0) {
        buffer[READ_BUFFER_SIZE - 1] = '\0';
        printf("%s", buffer);
    }
    putchar('\n');
}





int main(int argc, char* argv[]) {
    if (argc == 4 && strcmp(argv[1], "-r"))
        cp_dir_to_dir(argv[2], argv[3]);
    else if (argc == 3 &&)
    return 0;
}

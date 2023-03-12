#include <unistd.h>
#include <sys/stat.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>

#define READ_BUFFER_SIZE 33

void cat(const char* file_path) {
    struct stat st;
    if (stat(file_path, &st) == -1) {
        printf("cat: %s: No such file or directory\n", file_path);
        return;
    }
    if (S_ISDIR(st.st_mode)) {
        printf("cat: %s: Is a directory\n", file_path);
        return;
    }
    char buffer[READ_BUFFER_SIZE];
    int code = open(file_path, O_RDONLY);
    while (read(code, buffer, sizeof(buffer) - 1) > 0) {
        buffer[READ_BUFFER_SIZE - 1] = '\0';
        printf("%s", buffer);
    }
    putchar('\n');
    close(code);
}

int main(int argc, char* argv[]) {
    for (int file_idx = 1; file_idx < argc; ++file_idx) {
        cat(argv[file_idx]);
    }
    return 0;
}

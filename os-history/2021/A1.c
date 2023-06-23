// 实现命令 mygrep string file
// + 该命令逐行读取文件，如果行包括目标字符串，则打印该行
// + 该命令有两个命令行参数
//   - 参数 string，要搜索的字符串
//   - 参数 file，要查找的文件名
//
// 例子，在文件 /etc/passwd 中查找字符串 root，打印包含该字符串的行
// $ ./mygrep root /etc/passwd
// root:x:0:0:root:/root:/bin/bash
//
// 提示，可以使用函数 strstr 在字符串中查找字符串
// https://www.runoob.com/cprogramming/c-function-strstr.html

#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
char *string;
char *file;
int   main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Usage: %s string file\n", argv[0]);
        return 1;
    }
    string    = argv[1];
    file      = argv[2];
    FILE  *fp = fopen(file, "r");
    char   buff[1024];
    char  *line = buff;
    size_t readed, len;
    while ((readed = getline(&line, &len, fp)) != -1) {
        if (strstr(line, string)) {
            printf("%s", line);
        }
    }
    fclose(fp);
    return 0;
}
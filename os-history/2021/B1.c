// 通过 fork/exec/pipe/dup 实现 cat </etc/passwd | wc -l >result.txt
//
// + 父进程创建子进程
//   - 在父进程中实现功能 cat </etc/passwd
//   - 在子进程中实现功能 wc -l >result.txt
//   - 不能实现为
//     * 在子进程中实现功能 cat </etc/passwd
//     * 在父进程中实现功能 wc -l >result.txt
//
// + 该题不要求实现一个通用的 shell 程序
//   - 不需要使用 strtok 对字符串进行分割处理
//   - 假设字符串已经分割处理好了
//     * 父进程，使用 execlp("cat") 执行命令，使用 open("/etc/passwd") 打开文件
//     * 子进程，使用 execlp("wc") 执行命令，使用 open("result.txt") 打开文件
//
// + 请严格按要求完成
//   - 把作业 sh3.c 的内容复制过来，是没有分数的

#include <fcntl.h>
#include <stdio.h>
#include <sys/wait.h>
#include <unistd.h>
const char *pe = "cat";
const char *pv = NULL;
const char *pf = "/etc/passwd";
const char *ce = "wc";
const char *cf = "result.txt";
const char *cv = "-l";
int         pid;

int main() {
    if (fork() == 0) {
        int fd[2];
        pipe(fd);
        if ((fork()) == 0) {
            int w = open(cf, O_WRONLY | O_CREAT | O_TRUNC, 0644);
            dup2(fd[0], STDIN_FILENO);
            dup2(w, STDOUT_FILENO);
            close(fd[0]);
            close(fd[1]);
            close(w);
            execlp(ce, ce, cv, NULL);
        }
        int r = open(pf, O_RDONLY);
        dup2(r, STDIN_FILENO);
        dup2(fd[1], STDOUT_FILENO);
        close(fd[0]);
        close(fd[1]);
        close(r);
        execlp(pe, pe, pv, NULL);
    }
    wait(NULL);
    return 0;
}
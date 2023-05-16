#include "tcp.h"
#include "std.h"
#include <stdio.h>

#define CLOSE "\033[0m"    // 关闭彩色字体
#define RED "\033[31m"     // 红色字体
#define GREEN "\033[36m"   // 绿色字体
#define YELLOW "\033[33m"  // 黄色字体
#define BLUE "\033[34m"    // 蓝色字体

int tcp_listen(const char *ip_address, int port, int max_clients) {
    struct sockaddr_in server;
    int                fd;
    int                option;

    fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) panic("socket");

    option = 1;
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option)) < 0) panic("setsockopt");

    server.sin_family      = AF_INET;
    server.sin_addr.s_addr = inet_addr(ip_address);
    server.sin_port        = htons(port);
    if (bind(fd, (struct sockaddr *)&server, sizeof(struct sockaddr_in)) < 0) panic("bind");
    listen(fd, 8);
    return fd;
}

int tcp_accept(int server_fd) {
    struct sockaddr_in client;
    socklen_t          length = sizeof(struct sockaddr_in);
    int                client_fd;

    client_fd = accept(server_fd, (struct sockaddr *)&client, &length);
#ifdef DEBUG
    printf("[http-Web] " GREEN "client IP:" CLOSE "[%s], " GREEN "port:" CLOSE "[%d].\t" YELLOW "Connected\n" CLOSE, inet_ntoa(client.sin_addr), ntohs(client.sin_port));
#endif

    if (client_fd < 0) panic("accept");
    return client_fd;
}

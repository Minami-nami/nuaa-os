#include "echo.h"
#include <errno.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

#define MAXLISTEN 32

void clean(int signum) {
    while (waitpid(-1, NULL, WNOHANG) > 0)
        ;
}

int create_server(char *ip_address, int port) {
    struct sockaddr_in server;
    int                fd;
    int                option;

    fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) error("socket");

    option = 1;
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option)) < 0) error("setsockopt");

    server.sin_family      = AF_INET;
    server.sin_addr.s_addr = inet_addr(ip_address);
    server.sin_port        = htons(port);
    if (bind(fd, (struct sockaddr *)&server, sizeof(struct sockaddr_in)) < 0) error("bind");
    listen(fd, MAXLISTEN);
    signal(SIGCHLD, clean);
    return fd;
}

void echo_handler(int client_fd, int server_fd) {
    int pid;
    if ((pid = fork()) < 0)
        error("fork");
    else if (pid == 0) {
        close(server_fd);
        char buff[128];
        int  count  = recv(client_fd, buff, sizeof(buff), 0);
        buff[count] = 0;
        printf("server received: %s\n", buff);

        for (int i = 0; i < count; i++) buff[i] = toupper(buff[i]);
        write(client_fd, buff, count);
        close(client_fd);
        exit(EXIT_SUCCESS);
    }
    else {
        wait(NULL);
        close(client_fd);
    }
}

void run_echo_server(char *ip_address, int port) {
    int server_fd = create_server(ip_address, port);
    while (1) {
        struct sockaddr_in client;
        socklen_t          length = sizeof(struct sockaddr_in);
        int                client_fd;
        while ((client_fd = accept(server_fd, (struct sockaddr *)&client, &length)) < 0 && (errno == EINTR))
            ;
        if (client_fd < 0) error("accept");
        printf("accept client\n");

        echo_handler(client_fd, server_fd);
        close(client_fd);
    }
}

int main(int argc, char *argv[]) {
    char *ip_address = "127.0.0.1";
    int   port       = 1234;
    run_echo_server(ip_address, port);
    return 0;
}

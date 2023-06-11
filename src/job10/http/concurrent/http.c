#include "http.h"
#include "std.h"
#include "tcp.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>

#define BUF_SIZE 1024
#define METHOD_SIZE 256
#define URL_SIZE 256
#define PATH_SIZE 512

#define ROOT "www/"
#define _ROOT "www"
#define SERVER_STRING "Server: tiny httpd\r\n"

#define BAD_REQUEST 400
#define NOT_FOUND 404
#define NOT_IMPLEMENTED 501
#define INTERNAL_SERVER_ERROR 500

#define NO_EXPANSION "application/octet-stream"
#define HTML "text/html"
#define PLAIN "text/plain"
#define XML "text/xml"
#define GIF "image/gif"
#define JPEG "image/jpeg"
#define PNG "image/png"
#define ICO "image/x-icon"
#define SWF "application/x-shockwave-flash"
#define JS "application/javascript"
#define CSS "text/css"

#define CLOSE "\033[0m"    // 关闭彩色字体
#define RED "\033[31m"     // 红色字体
#define GREEN "\033[36m"   // 绿色字体
#define YELLOW "\033[33m"  // 黄色字体
#define BLUE "\033[34m"    // 蓝色字体

#define DIR_ITEM(client, buf, dir, name)                                    \
    sprintf(buf, "<li><a href=\"%s/%s\">%s</a></li>\r\n", dir, name, name); \
    send(client, buf, strlen(buf), 0)
#define DIR_HTML_HEADER(client, buf, dir)                                                                                              \
    sprintf(buf, "<html>\r\n<head>\r\n<title>Index of %s</title>\r\n</head>\r\n<body>\r\n<h1>Index of %s</h1>\r\n<ul>\r\n", dir, dir); \
    send(client, buf, strlen(buf), 0)
#define DIR_HTML_FOOTER(client, buf)                 \
    sprintf(buf, "</ul>\r\n</body>\r\n</html>\r\n"); \
    send(client, buf, strlen(buf), 0)

int   get_line(int sock, char *buf, int size);
char *get_expansion(const char *filename);
void  http_error_handler(int client, int status_code);
void  http_header(int client, const char *file_name, int is_dir, int cgi);
void  http_send_file(int client, const char *file_name);
void  http_send_dir(int client, const char *dir_name);
void *http_handler(void *arg);
void  execute_cgi(int client, const char *path, const char *method, const char *query_string);

int get_line(int sock, char *buf, int size) {
    int  n, i = 0;
    char c = '\0';

    while ((i < size - 1) && (c != '\n') && (n = recv(sock, &c, 1, 0)) > 0) {
        if (c == '\r') {
            n = recv(sock, &c, 1, MSG_PEEK);
            if ((n > 0) && (c == '\n'))
                recv(sock, &c, 1, 0);
            else
                c = '\n';
        }
        buf[i++] = c;
    }
    buf[i] = '\0';

    return i;
}

char *get_expansion(const char *filename) {
    char *dot = strrchr(filename, '.');
    if (!dot || dot == filename || dot + 1 == NULL) return NO_EXPANSION;
    ++dot;
    if (strcmp(dot, "htm") == 0 || strcmp(dot, "html") == 0) return HTML;
    if (strcmp(dot, "xml") == 0) return XML;
    if (strcmp(dot, "gif") == 0) return GIF;
    if (strcmp(dot, "jpeg") == 0 || strcmp(dot, "jpg") == 0) return JPEG;
    if (strcmp(dot, "png") == 0) return PNG;
    if (strcmp(dot, "ico") == 0) return ICO;
    if (strcmp(dot, "swf") == 0) return SWF;
    if (strcmp(dot, "js") == 0) return JS;
    if (strcmp(dot, "css") == 0) return CSS;
    return PLAIN;
}

void http_error_handler(int client, int status_code) {
    char buf[BUF_SIZE];
    switch (status_code) {
    case BAD_REQUEST:
        sprintf(buf, "HTTP/1.0 400 BAD REQUEST\r\n" SERVER_STRING "Content-type: text/html\r\n"
                     "\r\n"
                     "<P>Your browser sent a bad request, "
                     "such as a POST without a Content-Length.\r\n");
    case NOT_FOUND:
        sprintf(buf, "HTTP/1.0 404 NOT FOUND\r\n" SERVER_STRING "Content-Type: text/html\r\n"
                     "\r\n"
                     "<HTML><TITLE>Not Found</TITLE>\r\n"
                     "<BODY><P>The server could not fulfill\r\n"
                     "your request because the resource specified\r\n"
                     "is unavailable or nonexistent.\r\n"
                     "</BODY></HTML>\r\n");
    case NOT_IMPLEMENTED:
        sprintf(buf, "HTTP/1.0 501 Method Not Implemented\r\n" SERVER_STRING "Content-Type: text/html\r\n"
                     "\r\n"
                     "<HTML><HEAD><TITLE>Method Not Implemented\r\n"
                     "</TITLE></HEAD>\r\n"
                     "<BODY><P>HTTP request method not supported.\r\n"
                     "</BODY></HTML>\r\n");
    case INTERNAL_SERVER_ERROR:
        sprintf(buf, "HTTP/1.0 500 Internal Server Error\r\n" SERVER_STRING "Content-Type: text/html\r\n"
                     "\r\n"
                     "<P>Error prohibited CGI execution.\r\n");
    default:
        return;
    }
    send(client, buf, sizeof(buf), 0);
}

void http_header(int client, const char *filename, int is_dir, int cgi) {
    char buf[1024];
    (void)filename; /* could use filename to determine file type */

    strcpy(buf, "HTTP/1.0 200 OK\r\n" SERVER_STRING "Content-Type: ");
    strcat(buf, (is_dir || cgi) ? (HTML) : get_expansion(filename));
    strcat(buf, "\r\n\r\n");

    send(client, buf, strlen(buf), 0);
}

void http_send_file(int client, const char *file_name) {
    char buf[BUF_SIZE] = { 0, 0 };
    int  numchar;

    while ((numchar > 0) && strcmp("\n", buf)) numchar = get_line(client, buf, sizeof(buf));

    FILE *fp = fopen(file_name, "r");
    if (fp == NULL) {
        http_error_handler(client, NOT_FOUND);
        return;
    }
    http_header(client, file_name, 0, 0);

    fgets(buf, sizeof(buf), fp);
    while (!feof(fp)) {
        send(client, buf, strlen(buf), 0);
        fgets(buf, sizeof(buf), fp);
    }

    fclose(fp);
}

void http_send_dir(int client, const char *dir_name) {
    char buf[BUF_SIZE] = { 0, 0 };
    int  numchar;

    while ((numchar > 0) && strcmp("\n", buf)) numchar = get_line(client, buf, sizeof(buf));

    DIR *dir = opendir(dir_name);

    http_header(client, dir_name, 1, 0);

    struct dirent *entry;

    const char *dir_without_root = dir_name + strlen(ROOT) - 1;

    DIR_HTML_HEADER(client, buf, dir_without_root);

    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) continue;
        DIR_ITEM(client, buf, dir_without_root, entry->d_name);
    }

    DIR_HTML_FOOTER(client, buf);

    closedir(dir);
}

void *http_handler(void *arg) {
    int         client        = *(int *)arg;
    char        buf[BUF_SIZE] = { 0 }, method[METHOD_SIZE] = { 0 }, url[URL_SIZE] = { 0 }, path[PATH_SIZE] = { 0 };
    struct stat st;
    int         cgi = 0, numchars = 0, i = 0, j = 0;
    char       *query_string = NULL;

#ifdef DEBUG
    struct sockaddr_in client_addr;
    socklen_t          length = sizeof(struct sockaddr_in);
    getpeername(client, (struct sockaddr *)&client_addr, &length);
#endif

    numchars = get_line(client, buf, sizeof(buf));

    while (!isspace(buf[j]) && (i < METHOD_SIZE - 1)) {
        method[i++] = buf[j++];
    }
    method[i] = '\0';

    // METHOD
    if (strcasecmp(method, "POST") && strcasecmp(method, "GET")) {
        http_error_handler(client, NOT_IMPLEMENTED);
        goto THREAD_END;
    }

    if (strcasecmp(method, "POST") == 0) cgi = 1;

    i = 0;

    while (isspace(buf[j]) && (j < BUF_SIZE - 1)) j++;

    // URL
    while (!isspace(buf[j]) && (i < URL_SIZE - 1) && (j < BUF_SIZE - 1)) {
        url[i++] = buf[j++];
    }

    // PARSE PARAMS
    if (strcasecmp(method, "GET") == 0) {
        query_string = url;

        while ((*query_string != '?') && (*query_string != '\0')) query_string++;

        if (*query_string == '?') {
            cgi             = 1;
            *query_string++ = '\0';
        }
    }

#ifdef DEBUG
    printf("[http-Web] " GREEN "client IP:" CLOSE "[%s], " GREEN "port:" CLOSE "[%d].\t" BLUE "method: " CLOSE "%s, " BLUE "url: " CLOSE "%s, " BLUE "param: " CLOSE "%s\n",
           inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port), method, url, query_string);
#endif

    if (url[0] == '/')
        sprintf(path, _ROOT "%s", url);
    else
        sprintf(path, ROOT "%s", url);

    // URL IS EMPTY
    if (path[strlen(path) - 1] == '/') strcat(path, "index.html");

    if (stat(path, &st) == -1) {
        // IGNORE NEXT
        while ((numchars > 0) && strcmp("\n", buf)) numchars = get_line(client, buf, sizeof(buf));

        http_error_handler(client, NOT_FOUND);
        goto THREAD_END;
    }
    else {
        if (S_ISDIR(st.st_mode)) {
            http_send_dir(client, path);
#ifdef DEBUG
            printf("[http-Web] " GREEN "client IP:" CLOSE "[%s], " GREEN "port:" CLOSE "[%d].\t" BLUE "request content-type: " CLOSE "%s\n", inet_ntoa(client_addr.sin_addr),
                   ntohs(client_addr.sin_port), HTML);
#endif
            goto THREAD_END;
        }

        if ((st.st_mode & S_IXUSR) || (st.st_mode & S_IXGRP) || (st.st_mode & S_IXOTH)) cgi = 1;

        if (!cgi) {
            http_send_file(client, path);
#ifdef DEBUG
            printf("[http-Web] " GREEN "client IP:" CLOSE "[%s], " GREEN "port:" CLOSE "[%d].\t" BLUE "content-type: " CLOSE "%s\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port),
                   get_expansion(path));
#endif
            goto THREAD_END;
        }
        else
            execute_cgi(client, path, method, query_string);
    }

THREAD_END:
#ifdef DEBUG
    printf("[http-Web] " GREEN "client IP:" CLOSE "[%s], " GREEN "port:" CLOSE "[%d].\t" RED "Closed\n" CLOSE, inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
#endif
    close(client);
    return NULL;
}

// Creates a new httpd server listening on the specified IP address and port. The server will have a maximum of max_clients clients at any time.
int httpd_create(const char *ip, int port, int max_clients) {
    return tcp_listen(ip, port, max_clients);
}

// Runs the httpd server. This function will never return.
void httpd_run(int server) {
    while (1) {
        int       client = tcp_accept(server);
        pthread_t tid;
        pthread_create(&tid, NULL, http_handler, &client);
        pthread_detach(tid);
    }
}

void execute_cgi(int client, const char *path, const char *method, const char *query_string) {
    char  buf[BUF_SIZE] = { 0, 0 };
    int   cgi[2];
    pid_t pid;
    int   status;

    if (pipe(cgi) < 0) {
        http_error_handler(client, INTERNAL_SERVER_ERROR);
        return;
    }

    if ((pid = fork()) < 0) {
        http_error_handler(client, INTERNAL_SERVER_ERROR);
        return;
    }

    if (pid == 0) {
        char meth_env[255]   = {};
        char query_env[255]  = {};
        char length_env[255] = {};

        dup2(cgi[1], STDOUT_FILENO);

        close(cgi[0]);

        sprintf(meth_env, "REQUEST_METHOD=%s", method);
        putenv(meth_env);

        if (strcasecmp(method, "GET") == 0) {
            sprintf(query_env, "QUERY_STRING=%s", query_string);
            putenv(query_env);
        }
        else {
            sprintf(length_env, "CONTENT_LENGTH=%ld", strlen(query_string));
            putenv(length_env);
        }

        execl(path, path, NULL);
        exit(EXIT_FAILURE);
    }
    else {
        close(cgi[1]);

        http_header(client, path, 0, 1);
        int readed = 0;

        while ((readed = read(cgi[0], buf, sizeof(buf))) > 0) {
            send(client, buf, readed, 0);
        }
        send(client, "\r\n", 2, 0);

        close(cgi[0]);

        waitpid(pid, &status, 0);
#ifdef DEBUG
        if (status != 0)
            printf("[http-Web] " RED "CGI status: %d\n" CLOSE, status);
        else
            printf("[http-Web] " GREEN "CGI status: %d\n" CLOSE, status);
#endif
    }
}
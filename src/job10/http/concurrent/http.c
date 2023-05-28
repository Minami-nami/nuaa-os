#include "http.h"
#include "std.h"
#include "tcp.h"
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <sys/socket.h>
#include <sys/stat.h>

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
void  http_header(int client, const char *file_name, int is_dir);
void  http_send_file(int client, const char *file_name);
void  http_send_dir(int client, const char *dir_name);
void *http_handler(void *arg);

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

void http_header(int client, const char *filename, int is_dir) {
    char buf[1024];
    (void)filename; /* could use filename to determine file type */

    strcpy(buf, "HTTP/1.0 200 OK\r\n" SERVER_STRING "Content-Type: ");
    strcat(buf, is_dir ? (HTML) : get_expansion(filename));
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
    http_header(client, file_name, 0);

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

    http_header(client, dir_name, 1);

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
    int         client = *(int *)arg;
    char        buf[BUF_SIZE], method[METHOD_SIZE], url[URL_SIZE], path[PATH_SIZE];
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
    url[i] = '\0';

#ifdef DEBUG
    printf("[http-Web] " GREEN "client IP:" CLOSE "[%s], " GREEN "port:" CLOSE "[%d].\t" BLUE "method: " CLOSE "%s, " BLUE "url: " CLOSE "%s\n", inet_ntoa(client_addr.sin_addr),
           ntohs(client_addr.sin_port), method, url);
#endif

    if (strcasecmp(method, "GET")) {
        query_string = url;

        while ((*query_string != '?') && (*query_string != '\0')) query_string++;

        if (*query_string == '?') {
            cgi             = 1;
            *query_string++ = '\0';
        }
    }
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
            ;  // execute_cgi(client, path, method, query_string);
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
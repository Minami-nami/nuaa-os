#ifndef _TCP_H
#define _TCP_H
extern int tcp_listen(const char *ip_address, int port, int max_clients);
extern int tcp_accept(int server_fd);

#endif

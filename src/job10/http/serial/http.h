#ifndef _HTTP_H
#define _HTTP_H
#include <stdio.h>

extern int  debug_mode;
extern int  httpd_create(const char *ip, int port, int max_clients);
extern void httpd_run(int server);

#endif

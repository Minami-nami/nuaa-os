#include "http.h"
#include "std.h"
#include "tcp.h"
#include <sys/socket.h>

void usage() {
    puts("Usage: httpd -p port -h");
    puts("  -p port");
}

int main(int argc, char *argv[]) {
    const char *ip_address = "127.0.0.1";
    int         port       = 80;
    if (argc < 2) return usage(), 0;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-p") == 0) {
            port = atoi(argv[++i]);
            continue;
        }
    }
    int server = httpd_create(ip_address, port, 64);
    httpd_run(server);
    return 0;
}

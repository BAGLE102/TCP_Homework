/*
 * TCP Echo Client (Linux, IPv4) — line-based echo
 * Author : 况旻諭 (614430005)
 *
 * 編譯:  gcc -O2 -Wall -Wextra -pedantic -std=c11 -o tcp_client client.c
 * 執行:  ./tcp_client 127.0.0.1 5000
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>         // read(), write(), close()
#include <arpa/inet.h>      // inet_pton()
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>

#define BUF_SIZE 4096

static void die(const char *msg) { perror(msg); exit(EXIT_FAILURE); }

static void writeAll(int fd, const char *buf, size_t len) {
    size_t offset = 0;
    while (offset < len) {
        ssize_t n = write(fd, buf + offset, len - offset);
        if (n < 0) {
            if (errno == EINTR) continue;
            die("write");
        }
        offset += (size_t)n;
    }
}

/* 讀回一行（以 '\n' 作結束）。0 = 伺服器關閉；-1 = 錯誤。 */
static ssize_t readLine(int fd, char *out, size_t outSize) {
    if (outSize == 0) return -1;
    size_t used = 0;
    while (used < outSize - 1) {
        char c;
        ssize_t n = read(fd, &c, 1);
        if (n == 0) { if (used == 0) return 0; break; }
        if (n < 0) { if (errno == EINTR) continue; return -1; }
        out[used++] = c;
        if (c == '\n') break;
    }
    out[used] = '\0';
    return (ssize_t)used;
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <server_ip> <port>\n", argv[0]);
        return EXIT_FAILURE;
    }

    const char *serverIp = argv[1];
    int port = atoi(argv[2]);
    if (port <= 0 || port > 65535) {
        fprintf(stderr, "Invalid port: %s\n", argv[2]);
        return EXIT_FAILURE;
    }

    // 1) 建立 TCP socket
    int sockFd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockFd < 0) die("socket");

    // 2) 組 server 位址
    struct sockaddr_in serv;
    memset(&serv, 0, sizeof(serv));
    serv.sin_family = AF_INET;
    serv.sin_port   = htons((uint16_t)port);
    if (inet_pton(AF_INET, serverIp, &serv.sin_addr) != 1) {
        fprintf(stderr, "Invalid IP: %s\n", serverIp);
        close(sockFd);
        return EXIT_FAILURE;
    }

    // 3) 連線
    if (connect(sockFd, (struct sockaddr*)&serv, sizeof(serv)) < 0)
        die("connect");

    printf("[CLIENT] Connected to %s:%d\n", serverIp, port);

    char sendBuf[BUF_SIZE];
    char recvBuf[BUF_SIZE];

    // 4) 從 stdin 逐行讀 → 送出 → 讀回 echo → 顯示
    for (;;) {
        printf("Enter the string : ");
        fflush(stdout);

        if (!fgets(sendBuf, sizeof(sendBuf), stdin)) {  // EOF 或讀取錯誤
            printf("\n[CLIENT] Input closed.\n");
            break;
        }

        writeAll(sockFd, sendBuf, strlen(sendBuf));

        ssize_t n = readLine(sockFd, recvBuf, sizeof(recvBuf));
        if (n <= 0) {                       // 伺服器關閉或錯誤
            printf("[CLIENT] Server closed.\n");
            break;
        }

        printf("From Server : %s", recvBuf); // 與題目示意一致
    }

    close(sockFd);
    return 0;
}

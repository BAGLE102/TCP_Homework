/*
 * TCP Echo Server (Linux, IPv4) — line-based echo
 * Author : 况旻諭 (614430005)
 *
 * 用途:
 *   1) 監聽指定 port
 *   2) accept 一個連線
 *   3) 對於用戶端送來的每一行(以 '\n' 結尾)原封不動回傳
 *
 * 編譯:  gcc -O2 -Wall -Wextra -pedantic -std=c11 -o tcp_server server.c
 * 執行:  ./tcp_server 5000
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>         // close(), read(), write()
#include <arpa/inet.h>      // htons(), inet_ntop()
#include <netinet/in.h>     // sockaddr_in
#include <sys/socket.h>
#include <sys/types.h>

#define BACKLOG 16          // listen() 佇列長度
#define BUF_SIZE 4096       // 單次讀寫緩衝大小

static void die(const char *msg) { perror(msg); exit(EXIT_FAILURE); }

/* 將 buf 的前 len 位元組全部寫完（處理 write() 部分寫入） */
static void writeAll(int fd, const char *buf, size_t len) {
    size_t offset = 0;
    while (offset < len) {
        ssize_t written = write(fd, buf + offset, len - offset);
        if (written < 0) {
            if (errno == EINTR) continue; // 被訊號中斷就重試
            die("write");
        }
        offset += (size_t)written;
    }
}

/* 從 fd 讀取一行（包含 '\n'）。回傳讀到的位元組數；0 表示對方關閉；-1 表示錯誤。 */
static ssize_t readLine(int fd, char *out, size_t outSize) {
    if (outSize == 0) return -1;
    size_t used = 0;
    while (used < outSize - 1) {
        char ch;
        ssize_t bytesRead = read(fd, &ch, 1);
        if (bytesRead == 0) { // 對方關閉
            if (used == 0) return 0;
            break;
        }
        if (bytesRead < 0) {
            if (errno == EINTR) continue; // 重試
            return -1;
        }
        out[used++] = ch;
        if (ch == '\n') break;
    }
    out[used] = '\0';
    return (ssize_t)used;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <port>\n", argv[0]);
        return EXIT_FAILURE;
    }

    int port = atoi(argv[1]);
    if (port <= 0 || port > 65535) {
        fprintf(stderr, "Invalid port: %s\n", argv[1]);
        return EXIT_FAILURE;
    }

    // 1) 建立 TCP 監聽 socket
    int listenFd = socket(AF_INET, SOCK_STREAM, 0);
    if (listenFd < 0) die("socket");

    // SO_REUSEADDR: 重啟程式可快速重綁相同 port
    int opt = 1;
    if (setsockopt(listenFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
        die("setsockopt");

    // 2) 綁定 0.0.0.0:port
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons((uint16_t)port);
    if (bind(listenFd, (struct sockaddr*)&addr, sizeof(addr)) < 0)
        die("bind");

    // 3) 監聽
    if (listen(listenFd, BACKLOG) < 0)
        die("listen");

    printf("[SERVER] Listening on 0.0.0.0:%d\n", port);
    fflush(stdout);

    for (;;) {
        // 4) 等待用戶端
        struct sockaddr_in clientAddr;
        socklen_t clientLen = sizeof(clientAddr);
        int connFd = accept(listenFd, (struct sockaddr*)&clientAddr, &clientLen);
        if (connFd < 0) { perror("accept"); continue; }

        char clientIp[INET_ADDRSTRLEN] = {0};
        inet_ntop(AF_INET, &clientAddr.sin_addr, clientIp, sizeof(clientIp));
        printf("[SERVER] Connected: %s:%d\n", clientIp, ntohs(clientAddr.sin_port));

        // 5) 逐行回應
        char buffer[BUF_SIZE];
        for (;;) {
            ssize_t bytesRead = readLine(connFd, buffer, sizeof(buffer));
            if (bytesRead == 0) {
                printf("[SERVER] Client closed: %s:%d\n", clientIp, ntohs(clientAddr.sin_port));
                break;
            } else if (bytesRead < 0) {
                perror("read");
                break;
            }
            printf("From Client : %s", buffer);
            writeAll(connFd, buffer, (size_t)bytesRead);   // 原封不動回傳
        }

        close(connFd);
    }

    close(listenFd);
    return 0;
}

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
    #define WIN32_LEAN_AND_MEAN
    #include <windows.h>
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #pragma comment(lib, "Ws2_32.lib")
    typedef SOCKET socket_t;
    #define CLOSESOCKET closesocket
    #define CLEANUPWSA WSACleanup()

#else
    #include <sys/types.h>
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <unistd.h>
    #include <pthread.h>
    typedef int socket_t;
    #define INVALID_SOCKET -1
    #define SOCKET_ERROR -1
    #define CLOSESOCKET close
    #define CLEANUPWSA ((void)0)

#endif

#define PORT 8080
#define BACKLOG 10
#define RECV_BUF_SIZE 4096

/* Loaded at runtime in main() */
char *html = NULL;
long filesize = 0;

struct client_info{
    socket_t client_fd;
    struct sockaddr_in client_addr;
};

#ifdef _WIN32
    DWORD WINAPI client_thread(LPVOID arg)
#else
    void* client_thread(void* arg)
#endif
{
    struct client_info *info = (struct client_info *)arg;
    socket_t client_fd = info->client_fd;

    char buf[RECV_BUF_SIZE];
    int bytes = recv(client_fd, buf, sizeof(buf) - 1, 0);
    if (bytes > 0){
        buf[bytes] = '\0';
        printf("Received request: \n%s\n", buf);
        /* Build response header with actual content length */
        char header[256];
        int header_len = snprintf(header, sizeof(header),
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: text/html\r\n"
            "Content-Length: %ld\r\n"
            "Connection: close\r\n"
            "\r\n", filesize);
        if (header_len > 0) {
            send(client_fd, header, header_len, 0);
        }
        if (html && filesize > 0) {
            send(client_fd, html, (int)filesize, 0);
        }
    }

    CLOSESOCKET(client_fd);
    free(info);
#ifdef _WIN32
    return 0;
#else
    return NULL;
#endif
}

int main(void){
    int result;

#ifdef _WIN32
    WSADATA wsaData;
    result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (result != 0){
        fprintf(stderr, "WSAStartup failed: %d\n", result);
        return 1;
    }

#endif
    socket_t server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == INVALID_SOCKET){
        perror("socket");
        CLEANUPWSA;
        return 1;
    }

#ifndef _WIN32
    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1){
        perror("setsockopt");
        CLOSESOCKET(server_fd);
        CLEANUPWSA;
        return 1;
    }

#endif
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    result = bind(server_fd, (struct sockaddr *)&addr, sizeof(addr));
    if (result == SOCKET_ERROR){
        perror("bind");
        CLOSESOCKET(server_fd);
        CLEANUPWSA;
        return 1;
    }

    result = listen(server_fd, BACKLOG);
    if (result == SOCKET_ERROR){
        perror("listen");
        CLOSESOCKET(server_fd);
        CLEANUPWSA;
        return 1;
    }

    printf("Concurrent HTTP server listening on port %d ...\n", PORT);

    /* Load index.html into memory once */
    FILE *fp = fopen("index.html", "rb");
    if (!fp) {
        perror("fopen");
        CLOSESOCKET(server_fd);
        CLEANUPWSA;
        return 1;
    }
    fseek(fp, 0, SEEK_END);
    filesize = ftell(fp);
    rewind(fp);
    html = malloc((size_t)filesize);
    if (!html) {
        perror("malloc");
        fclose(fp);
        CLOSESOCKET(server_fd);
        CLEANUPWSA;
        return 1;
    }
    if (fread(html, 1, (size_t)filesize, fp) != (size_t)filesize) {
        perror("fread");
        free(html);
        fclose(fp);
        CLOSESOCKET(server_fd);
        CLEANUPWSA;
        return 1;
    }
    fclose(fp);

    while(1){
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        socket_t client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_len);
        if (client_fd == INVALID_SOCKET){
            perror("accept");
            break;
        }

        struct client_info *info = malloc(sizeof(*info));
        if (!info){
            fprintf(stderr, "malloc failed\n");
            CLOSESOCKET(client_fd);
            continue;
        }

        info->client_fd = client_fd;
        info->client_addr = client_addr;

#ifdef _WIN32
        HANDLE hthread = CreateThread(
            NULL,
            0,
            client_thread, 
            info,
            0,
            NULL
        );
        if (hthread == NULL){
            fprintf(stderr, "CreateThread failed\n");
            CLOSESOCKET(client_fd);
            free(info);
            continue;
        }

        CloseHandle(hthread);
#else
        pthread_t tid;
        if (pthread_create(&tid, NULL, client_thread, info) != 0){
            perror("pthread_create");
            CLOSESOCKET(client_fd);
            free(info);
            continue;
        }
        pthread_detach(tid);

#endif
        }
        free(html);
        CLOSESOCKET(server_fd);
        CLEANUPWSA;
        return 0;

}
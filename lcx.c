#ifdef _WIN32
#include <winsock2.h>
#include <windows.h>
#pragma comment(lib, "ws2_32.lib")
typedef int socklen_t;
#else
#include <arpa/inet.h>
#include <netinet/in.h>
#include <pthread.h>
#include <sys/socket.h>
#include <unistd.h>
#define SOCKET int
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
#define closesocket close
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BUF_SIZE 8192

typedef struct {
    SOCKET client;
    SOCKET remote;
} relay_pair;

void *relay_thread(void *arg)
{
    relay_pair *pair = (relay_pair *)arg;
    char buf[BUF_SIZE];

    fd_set rfds;

    while (1)
    {
        FD_ZERO(&rfds);
        FD_SET(pair->client, &rfds);
        FD_SET(pair->remote, &rfds);

        int maxfd = pair->client > pair->remote ? pair->client : pair->remote;

        int ret = select(maxfd + 1, &rfds, NULL, NULL, NULL);

        if (ret <= 0)
            break;

        // client -> remote
        if (FD_ISSET(pair->client, &rfds))
        {
            int len = recv(pair->client, buf, BUF_SIZE, 0);
            if (len <= 0)
                break;

            if (send(pair->remote, buf, len, 0) <= 0)
                break;
        }

        // remote -> client
        if (FD_ISSET(pair->remote, &rfds))
        {
            int len = recv(pair->remote, buf, BUF_SIZE, 0);
            if (len <= 0)
                break;

            if (send(pair->client, buf, len, 0) <= 0)
                break;
        }
    }

    closesocket(pair->client);
    closesocket(pair->remote);

    free(pair);

    printf("[-] Connection closed\n");

    return NULL;
}

int main(int argc, char *argv[])
{
    if (argc != 4)
    {
        printf("Usage:\n");
        printf("%s <listen_port> <target_ip> <target_port>\n", argv[0]);
        printf("Example:\n");
        printf("%s 5555 192.168.1.10 3389\n", argv[0]);
        return 1;
    }

#ifdef _WIN32
    WSADATA wsa;
    WSAStartup(MAKEWORD(2,2), &wsa);
#endif

    int listen_port = atoi(argv[1]);
    char *target_ip = argv[2];
    int target_port = atoi(argv[3]);

    SOCKET listen_sock = socket(AF_INET, SOCK_STREAM, 0);

    if (listen_sock == INVALID_SOCKET)
    {
        printf("socket failed\n");
        return 1;
    }

    int opt = 1;
    setsockopt(listen_sock, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt));

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));

    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(listen_port);

    if (bind(listen_sock, (struct sockaddr *)&addr, sizeof(addr)) == SOCKET_ERROR)
    {
        printf("bind failed\n");
        return 1;
    }

    if (listen(listen_sock, 20) == SOCKET_ERROR)
    {
        printf("listen failed\n");
        return 1;
    }

    printf("[+] Listening on port %d\n", listen_port);
    printf("[+] Forwarding to %s:%d\n", target_ip, target_port);

    while (1)
    {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);

        SOCKET client_sock = accept(listen_sock,
                                    (struct sockaddr *)&client_addr,
                                    &client_len);

        if (client_sock == INVALID_SOCKET)
            continue;

        printf("[+] Client connected\n");

        SOCKET remote_sock = socket(AF_INET, SOCK_STREAM, 0);

        if (remote_sock == INVALID_SOCKET)
        {
            closesocket(client_sock);
            continue;
        }

        struct sockaddr_in remote_addr;
        memset(&remote_addr, 0, sizeof(remote_addr));

        remote_addr.sin_family = AF_INET;
        remote_addr.sin_port = htons(target_port);
        remote_addr.sin_addr.s_addr = inet_addr(target_ip);

        if (connect(remote_sock,
                    (struct sockaddr *)&remote_addr,
                    sizeof(remote_addr)) == SOCKET_ERROR)
        {
            printf("[-] Connect target failed\n");
            closesocket(client_sock);
            closesocket(remote_sock);
            continue;
        }

        printf("[+] Connected to target\n");

        relay_pair *pair = (relay_pair *)malloc(sizeof(relay_pair));
        pair->client = client_sock;
        pair->remote = remote_sock;

#ifdef _WIN32
        HANDLE th = CreateThread(NULL,
                                 0,
                                 (LPTHREAD_START_ROUTINE)relay_thread,
                                 pair,
                                 0,
                                 NULL);
        CloseHandle(th);
#else
        pthread_t tid;
        pthread_create(&tid, NULL, relay_thread, pair);
        pthread_detach(tid);
#endif
    }

    closesocket(listen_sock);

#ifdef _WIN32
    WSACleanup();
#endif

    return 0;
}

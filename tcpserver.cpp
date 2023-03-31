#ifdef _WIN32 
#define _CRT_SECURE_NO_WARNINGS
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winsock2.h>
// Директива линковщику: использовать библиотеку сокетов
#pragma comment(lib, "ws2_32.lib")

#include <ws2tcpip.h>
#include <stdint.h>
#else // LINUX
#include <sys/types.h> 
#include <sys/socket.h> 
#include <netdb.h> 
#include <errno.h>
#include <unistd.h>
#endif
#include <stdio.h> 
#include <string.h>

#include <stdlib.h> 
#define LEN 400000
#define MX 70
int sock_err(const char* function, int s)
{
    int err;
#ifdef _WIN32
    err = WSAGetLastError();
#else
    err = errno;
#endif
    fprintf(stderr, "%s: socket error: %d\n", function, err);
    return -1;
}
int init()
{
#ifdef _WIN32
    // Для Windows следует вызвать WSAStartup перед началом использования сокетов
    WSADATA wsa_data;
    return (0 == WSAStartup(MAKEWORD(2, 2), &wsa_data));
#else
    return 1; // Для других ОС действий не требуется
#endif
}
void deinit()
{
#ifdef _WIN32
    // Для Windows следует вызвать WSACleanup в конце работы
    WSACleanup();
#else
    // Для других ОС действий не требуется
#endif
}

void s_close(int s)
{
#ifdef _WIN32
    closesocket(s);
#else
    close(s);
#endif
}
int stop_fl = 1;
int n_mes;

int put = 0;

int parse_and_print(unsigned int ip, unsigned short port, int cs, FILE* f)
{
    char buf[LEN];
    //memset(buf, 0, LEN);
    int len;
    len = recv(cs, buf, 4, 0);
    //send(cs, "1", 1, 0);
    if (len == -1)
        return 0;

    buf[len] = 0;
    int n_mes = (unsigned char)buf[3];
    n_mes = (unsigned char)buf[2] + (n_mes << 8);
    n_mes = (unsigned char)buf[1] + (n_mes << 8);
    n_mes = (unsigned char)buf[0] + (n_mes << 8);
    n_mes = ntohl(n_mes);
    fprintf(f, "%u.%u.%u.%u:", (ip >> 24) & 0xFF, (ip >> 16) & 0xFF, (ip >> 8) & 0xFF, (ip) & 0xFF);
    fprintf(f, "%d ", port);
    //AA
    len = recv(cs, buf, 2, 0);
    if (len == -1)
        return 0;
    buf[len] = 0;
    uint16_t AA = (unsigned char)buf[0];
    AA += (buf[1] << 8);
    AA = ntohs(AA);
    fprintf(f, "%d ", AA);
    //printf("AA = %d\n", AA);

    //BBB
    //printf("BBB start\n");
    len = recv(cs, buf, 4, 0);
    if (len == -1)
        return 0;
    buf[len] = 0;
    //printf("BBB end\n");
    int BBB = (unsigned char)buf[3];
    BBB = (unsigned char)buf[2] + (BBB << 8);
    BBB = (unsigned char)buf[1] + (BBB << 8);
    BBB = (unsigned char)buf[0] + (BBB << 8);
    BBB = ntohl(BBB);
    fprintf(f, "%d ", BBB);
    //printf("BBB = %d\n", BBB);

    //hh
    len = recv(cs, buf, 1, 0);
    if (len == -1)
        return 0;
    buf[len] = 0;
    char hh = atoi(buf);// = buf[0];
    fprintf(f, "%.2d:", buf[0]);
    //printf("hh = %d\n", buf[0]);

    //mm
    len = recv(cs, buf, 1, 0);
    if (len == -1)
        return 0;
    buf[len] = 0;
    char mm = atoi(buf);// = buf[0];
    fprintf(f, "%.2d:", buf[0]);
    //printf("mm = %d\n", buf[0]);

    //ss
    len = recv(cs, buf, 1, 0);
    if (len == -1)
        return 0;
    buf[len] = 0;
    char ss = atoi(buf);// = buf[0];
    fprintf(f, "%.2d ", buf[0]);
    //printf("ss = %d\n", buf[0]);

    //N
    len = recv(cs, buf, 4, 0);
    if (len == -1)
        return 0;
    int N = (unsigned char)buf[3];
    N = (unsigned char)buf[2] + (N << 8);
    N = (unsigned char)buf[1] + (N << 8);
    N = (unsigned char)buf[0] + (N << 8);
    N = ntohl(N);
    //printf("BBBstr = %s\n", buf);
    //printf("N = %d\n", N);

    //message
    unsigned int sum = 0;
    while (N > sum)
    {
        len = recv(cs, buf, N, 0);
        sum += len;
        if (len == -1)
            return 0;
        //printf("er\n");
        buf[len] = 0;
        fprintf(f, "%s", buf);
    }
    fprintf(f, "\n");
    //printf("%s\n", buf);
    //send(cs, "ok", 2, 0);
    len = send(cs, "ok", 2, 0);
    printf("message recived\n");
    if (len < 0)
        printf("Error: send ok");

    if (!strcmp("stop", buf))
    {
        stop_fl = 0;
        return 0;
    }
    return 1;

}

int set_non_block_mode(int s)
{
#ifdef _WIN32
    unsigned long mode = 1;
    return ioctlsocket(s, FIONBIO, &mode);
#else
    int fl = fcntl(s, F_GETFL, 0);
    return fcntl(s, F_SETFL, fl | O_NONBLOCK);
#endif
}

void close_sockets(int* cs, int cnt)
{
    //закрыть все сокеты

}



int main(int argc, char** argv)
{
    if (argc != 2)
    {
        return sock_err("argument", 0);
    }
    n_mes = 0;
    init();
    int cs_cnt = 0;

    
    int ls; // Сокет, прослушивающий соединения  
    int cs[MX] = { 0 }; // Connected client
    // Заполнение адреса прослушивания 
    ls = socket(AF_INET, SOCK_STREAM, 0);
    if (ls < 0)
        return sock_err("socket", ls);
    if (set_non_block_mode(ls) != 0)
        return sock_err("\nset_non_block_mode  %s\n", ls);

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(atoi(argv[1])); // Сервер прослушивает порт 9000  
    addr.sin_addr.s_addr = htonl(INADDR_ANY); // Все адреса

    // Связывание сокета и адреса прослушивания  
    if (bind(ls, (struct sockaddr*)&addr, sizeof(addr)) < 0)
        return sock_err("bind", ls);
    HANDLE events[2];
    events[0] = WSACreateEvent();
    events[1] = WSACreateEvent();
    WSAEventSelect(ls, events[0], FD_ACCEPT);
    int i;
    char fl[MX] = { 0 };
    if (listen(ls, MX) < 0)
        return sock_err("listen", ls);
    for (i = 0; i < cs_cnt; i++)
        WSAEventSelect(cs[i], events[1], FD_READ | FD_WRITE | FD_CLOSE);
    WSANETWORKEVENTS ne;
    unsigned short port = atoi(argv[1]);
    char buf[4];
    int len = 0;
    FILE* f = fopen("msg.txt", "a+");
    unsigned int ip[MX];

    char c_cl_cn[MX] = { 0 };
    int addrlen = 0;
    while (stop_fl)
    {

        // Ожидание событий в течение секунды
        DWORD dw = WSAWaitForMultipleEvents(2, events, FALSE, 1000, FALSE);
        WSAResetEvent(events[0]);
        WSAResetEvent(events[1]);

        if (0 == WSAEnumNetworkEvents(ls, events[0], &ne) && (ne.lNetworkEvents & FD_ACCEPT))
        {
            addrlen = sizeof(addr);
            c_cl_cn[cs_cnt] = 1;
            cs[cs_cnt] = accept(ls, (struct sockaddr*)&addr, &addrlen);
            ip[cs_cnt] = ntohl(addr.sin_addr.s_addr);
            WSAEventSelect(cs[cs_cnt], events[1], FD_READ | FD_WRITE | FD_CLOSE);
            //fprintf(f, "Client connected: %u.%u.%u.%u\n", (ip >> 24) & 0xFF, (ip >> 16) & 0xFF, (ip >> 8) & 0xFF, (ip) & 0xFF);
            ne.lNetworkEvents = FD_READ | FD_WRITE;

            printf("Client connected: %u.%u.%u.%u\n", (ip[cs_cnt] >> 24) & 0xFF, (ip[cs_cnt] >> 16) & 0xFF, (ip[cs_cnt] >> 8) & 0xFF, (ip[cs_cnt]) & 0xFF);
            cs_cnt++;
        }
        for (i = 0; i < cs_cnt; i++)
        {
            if (0 == WSAEnumNetworkEvents(cs[i], events[1], &ne))
            {
                if (ne.lNetworkEvents & FD_CLOSE)
                {
                    c_cl_cn[i] = 0;
                    cs[i] = 0;
                    closesocket(cs[i]);
                    printf("Client: %u.%u.%u.%u:%d disconnected\n", (ip[i] >> 24) & 0xFF, (ip[i] >> 16) & 0xFF, (ip[i] >> 8) & 0xFF, (ip[i]) & 0xFF, port);
                }
                if (c_cl_cn[i] && (ne.lNetworkEvents & FD_READ))
                {
                    if (fl[i] == 0)
                    {
                        len = recv(cs[i], buf, 3, 0);
                        buf[len] = 0;
                        if (len == -1)
                            return 0;
                        if (!strcmp(buf, "put"))
                        {
                            printf("READY\n");

                        }
                        else
                        {
                            stop_fl = 0;
                            break;
                        }
                        fl[i] = 1;
                    }
                    if (!parse_and_print(ip[i], port, cs[i], f))
                    {
                        break;
                    }
                }
                


            }
        }



    }
    for (i = 0; i < cs_cnt; i++)
    {
        if (cs[i] != 0)
            closesocket(cs[i]);
    }

    deinit();

    return 0;
}
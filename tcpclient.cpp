#ifdef _WIN32 
#define WIN32_LEAN_AND_MEAN
    #include <windows.h> 
    #include <winsock2.h>
    #include <ws2tcpip.h>
    // Директива линковщику: использовать библиотеку сокетов
    #pragma comment(lib, "ws2_32.lib")
#else // LINUX
    #include <sys/types.h> 
    #include <sys/socket.h>
    #include <netdb.h> 
    #include <errno.h>
    #include <unistd.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
#endif

#include <stdio.h>
#include <stdlib.h> 
#include <string.h> 

#define LEN 400000

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

void s_close(int s)
{
#ifdef _WIN32
    closesocket(s);
#else
    close(s);
#endif
}

struct ls_mes
{
    char sym;
    ls_mes* next;
}message;


int wait_ans(int s)
{
    char buffer[256];
    int res;
    // Принятие очередного блока данных. // Если соединение будет разорвано удаленным узлом recv вернет 0
    while ((res = recv(s, buffer, sizeof(buffer), 0)) > 0) 
    {
        printf("  %d bytes received\n", res);
    }
    if(!strcmp(buffer, "ok"))
        return sock_err("recv", s);
    if (res < 0)
        return sock_err("recv", s);
    return 0;
}

union num
{
    int32_t i;
    char arr[4];
};

void slh_n(int s, union num numb, int flags)// string number
{
    numb.i = htonl(numb.i);
    send(s,numb.arr,4,flags);
}

int KOL = 0;

// Отправляет http-запрос на удаленный сервер
int send_request(int s, char* filename)
{
    const char* request = "put";
    int size = strlen(request);
    int sent = 0;
    union num numb;
    union num BBB, N;
    numb.i = 0;

#ifdef _WIN32
    int flags = 0;
#else
    int flags = MSG_NOSIGNAL;
#endif
    int res = send(s, request, size, flags);
    //send(s, "u", 1, flags);
    //send(s, "t", 1, flags);
    if(res<0)
    {
        return sock_err("send", s);
    }
    FILE* f = fopen(filename, "r");
    if(f == NULL)
    {
        return sock_err("file", s);
    }
    int i = 0;
    u_int32_t j = 0;

    char AA[6];
    char hh;
    char mm;
    char ss;
    char message[LEN];
    char count = 0;
    char buf[20];
    u_int16_t tmp;
    N.i = 0;
    unsigned int len;
    slh_n(s, numb, flags);
    //numb.i = htonl(numb.i);
    //send(s,numb.arr,4,flags);
    while((i = fgetc(f)) != EOF)
    {
        if((i != ' ' && count <5) && ((count <2 && count>4) || i != ':') && i != '\n')
        {
            buf[j] = i;
            j++;

        }
        else
        {
            //printf("count = %d\n", count);
            if(i == '\n')
                continue;
            if ( count == 0)// AA
            {
                buf[j] = 0;
                
                tmp = atoi(buf);
                //printf("tmp = %d\n",tmp);
                tmp = htons(tmp);
                //printf("tmp = %d\n",tmp);
                buf[1] = tmp >> 8;
                //printf("buf[1] = %d\n",buf[1] );
                buf[0] = (tmp<<8)>>8;
                //printf("buf[0] = %d\n",buf[0] );
                //buf[2] = 0;
                //printf("bufAA = %s\n", buf);
                send(s,buf,2, flags);
                //recv(s, buf, 1,flags);
                j = 0;
                count++;
            }
            else if (count == 1)// BBB
            {
                buf[j] = 0;
               
                BBB.i = atoi(buf);
                 
                slh_n(s, BBB, flags);
                //printf("arrBBB = %s\n", BBB.arr);
                j = 0;
                count++;
            }
            else if( count >=2 && count<=4)
            {
                buf[j] = 0;
                buf[0] = atoi(buf);
                //printf("buf = %s\n", buf);
                send(s, &buf[0], 1, flags);
                j = 0;
                count++;
            }
            else if( count == 5)
            {
                N.i = 0;
                j = 0;
                message[j++] = i;
                
                while((i = fgetc(f)) != '\n' && i != EOF)
                {
                    message[j++] = i;
                }
                
                message[j] =0;
                //printf("mes = %s\n", message);
                N.i = j;
                slh_n(s, N, flags);
                len = send(s, message, j, flags);
                
                //printf("wait\n");     
                len = recv(s, buf, 2, flags);
                if (len == 1)
                {
                    printf ("ok");
                    recv(s, buf, 1, 0);
                }   
                buf[len] = 0;
                //printf("%s\n", buf);
                /*
                while(strcmp("ok", buf))
                {
                    buf[len] = 0;
                    recv(s, buf, 2, flags);
                }
                */
                //printf("end wait\n");
                printf("message sent\n");
                KOL = numb.i+1;
                if(!strcmp(message,"stop"))
                {
                    //printf("%d messages sent\n", numb.i);
                    fclose(f);
                    return 0;
                }
               
                count = 0;
                j = 0;
                numb.i++;
                //KOL = numb.i;
                slh_n(s, numb, flags);
            }
        }

    }
    fclose(f);
    return 0;
}

int main(int argc, char* argv[])
{
    char str_ipaddr[20];
    char str_port[10];
    char* str_filename = NULL;

    if(argc != 3)
    {
        return sock_err("arguments", 0);
    }
    int i = 0;
    while((argv[1][i] != ':') && (argv[1][i] != ' ') && (argv[1][i] != 0) )
    {
        str_ipaddr[i] = argv[1][i];
        i++;
    }
    str_ipaddr[i] = 0;
    i++;
    int j =0;
    while(argv[1][i] != ':' && argv[1][i] != ' ' && argv[1][i] != 0)
    {
        str_port[j] = argv[1][i];
        i++;
        j++;
    }
    str_port[j] = 0;
    str_filename = argv[2];

    //int 
    int s;
    struct sockaddr_in addr; 
    FILE* f;
    // Инициалиазация сетевой библиотеки
    init();
    // Создание TCP-сокета
    s = socket(AF_INET, SOCK_STREAM, 0);
    if (s < 0)
        return sock_err("socket", s);
    // Заполнение структуры с адресом удаленного узла
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;//IPv4
    addr.sin_port = htons((u_int16_t)atoi(str_port));
    i = 0;
    if(inet_aton(str_ipaddr, &addr.sin_addr) == 0)
        return sock_err("ip_addr", s);
    // Установка соединения с удаленным хостом
    i = 0;
    while (connect(s, (struct sockaddr*) &addr, sizeof(addr)) != 0) 
    {
        i++;
        if(i == 10)
        {
            s_close(s);
            return sock_err("connect", s);
        }
        //printf("%d\n", i);
        usleep(100);
    }
    // Отправка запроса на удаленный сервер
    send_request(s, argv[2]);//argv[2] = filename
    // Прием результата
    //recv_response(s, f);

    // Закрытие соединения
    close(s);
    close(s);
    close(s);
    printf("MESSAGES = %d\n", KOL);
    deinit();
    return 0;
}
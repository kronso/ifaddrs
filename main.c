#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#include <pthread.h>
#include <ncurses.h>

#include <netdb.h>
#include <ifaddrs.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <poll.h>

#define UNAME_MAX 9
#define LAN_PORT_SERVER "4444"
#define PLAYER_LIMIT 2

char host_ipv4[INET_ADDRSTRLEN];
char broadcast[INET_ADDRSTRLEN];

typedef enum _State
{
    SEND_BROADCAST = 1,
    END_BROADCAST
} State;
State state = SEND_BROADCAST;

struct PlayerInfo
{
    char uname[UNAME_MAX]; 
    char server_ip[INET_ADDRSTRLEN]; 
    in_port_t port; 

    struct sockaddr_in* client_addr;
    socklen_t client_sz;
};
struct PlayerInfo players[PLAYER_LIMIT];
short nplayers = 0;

struct ServerInfo 
{ 
    char uname[UNAME_MAX]; 
    char server_ip[INET_ADDRSTRLEN]; 
    in_port_t port; 
};

void join_server(void)
{
    int socket_fd, res;
    struct addrinfo* result, hints;

    hints = (struct addrinfo)
    {
        .ai_family   = AF_INET,
        .ai_socktype = SOCK_DGRAM,
        .ai_protocol = IPPROTO_UDP,
    };

    if (getaddrinfo(broadcast, LAN_PORT_SERVER, &hints, &result) != 0)
    {
        fprintf(stderr, "Error %d: -- %s\n", errno, gai_strerror(errno));
        exit(EXIT_FAILURE);
    }

    socket_fd = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (socket_fd == -1)
        perror("socket");

    res = bind(socket_fd, result->ai_addr, result->ai_addrlen);
    if (res == -1)
        perror("bind");

    res = recvfrom(socket_fd, &players[1], sizeof(struct PlayerInfo), 0, result->ai_addr, &result->ai_addrlen);
    if (res == -1)
        perror("recvfrom");
    nplayers++;
    shutdown(socket_fd, SHUT_RD);
    close(socket_fd);


    // Bind to the server ip that was given from the broadcast


    char port_str[6] = { 0 };
    sprintf(port_str, "%d", players[1].port);
    if (getaddrinfo(players[1].server_ip, port_str, &hints, &result) != 0)
    {
        fprintf(stderr, "Error %d: -- %s\n", errno, gai_strerror(errno));
        exit(EXIT_FAILURE);
    }

    socket_fd = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (socket_fd == -1)
        perror("socket");

    res = bind(socket_fd, result->ai_addr, result->ai_addrlen);
    if (res == -1)
        perror("bind");

    res = sendto(socket_fd, &players[0], sizeof(struct PlayerInfo), 0, result->ai_addr, result->ai_addrlen);
    if (res == -1)
        perror("sendto");
    while (1)
    {
        printf("waiting...\n");
        sleep(5);
        // res = recvfrom(socket_fd, server_info);
    }

    freeaddrinfo(result);
    shutdown(socket_fd, SHUT_RD);
    close(socket_fd);
}

void start_server(void)
{
    int server_fd, res;
    struct sockaddr_in server_addr =
    {
        .sin_family = AF_INET,
        .sin_port = htons(2000),
        .sin_addr = INADDR_ANY
    };
    // if (inet_pton(AF_INET, host_ipv4, &server_addr.sin_addr) == -1)
    //     perror("inet_pton");
    
    // players[0] refers to self
    players[0].port = ntohs(server_addr.sin_port);
    nplayers++;

    server_fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (server_fd == -1)
        perror("server_fd");

    res = bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr));
    if (res == - 1)
        perror("bind");

    struct pollfd fds =
    {
        .fd = server_fd,
        .events = POLLIN,
    }; 
    nfds_t nfds = 1;

    printf("\nServer started...\n");
    while (1)
    {
        res = poll(&fds, nfds, -1);
        if (res == -1)
            perror("poll");
        
        // if (res == 0)
        //     continue;

        if (nplayers != PLAYER_LIMIT)
        {
            if (fds.revents & POLLIN)
            {
                res = recvfrom(server_fd, &players[nplayers], sizeof(struct PlayerInfo), 0, (struct sockaddr*)&players[nplayers].client_addr, &players[nplayers].client_sz);
                if (res == -1)
                    perror("recvfrom");
                printf("Received confirmation...\n");
                nplayers++;

            }
            if (fds.revents & POLLHUP)
            {
            }
        }
        if (nplayers == PLAYER_LIMIT)
        {
            state = (state == SEND_BROADCAST) ? END_BROADCAST: state;
            int input_size = 0;
            if (fds.revents & POLLIN)
            {
                struct sockaddr_in* client_addr = NULL;
                socklen_t client_sz;

                res = recvfrom(server_fd, &input_size, sizeof(int), 0, (struct sockaddr *)client_addr, &client_sz);
                if (res == -1)
                    perror("recvfrom");

                for (int i = 0; i < nplayers; i++)
                {
                    // Send data to all players except itself
                    if (players[i].client_addr->sin_addr.s_addr != client_addr->sin_addr.s_addr)
                    {
                        sendto(server_fd, &input_size, sizeof(int), 0, (struct sockaddr *)players[i].client_addr, players[i].client_sz);
                    }
                }
            }
            if (fds.revents & POLLHUP)
            {
            }
        }
    }
}

void handle_server(void)
{
    pthread_t th;
    pthread_create(&th, NULL, (void *)&start_server, NULL);
}

void send_broadcast(void)
{
    int server_fd, res;
    struct sockaddr_in server_addr =
    {
        .sin_family = AF_INET,
        .sin_port = htons(atoi(LAN_PORT_SERVER)),
    };
    // Directed broadcast
    if (inet_pton(AF_INET, broadcast, &server_addr.sin_addr) == -1)
        perror("inet_pton");

    server_fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (server_fd == -1)
        perror("server_fd");

    res = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_BROADCAST, &res, sizeof(res));
    
    printf("\nSending Broadcast...\n");
    while (nplayers != PLAYER_LIMIT || state == SEND_BROADCAST)
    {
        res = sendto(server_fd, &players[0], sizeof(struct PlayerInfo), 0, (struct sockaddr *)&server_addr, sizeof(server_addr));
        if (res == -1)
            perror("sendto");
        // send every 500 miliseconds (keep it around 100 to 500ms)
        usleep(500000);
    }  

    // res = sendto(server_fd, &state, sizeof(int), 0, (struct sockaddr *)&server_addr, sizeof(server_addr));

    printf("Stop sending broadcast...\n");
    shutdown(server_fd, SHUT_WR);
    close(server_fd);  
}   

void handle_broadcast(void)
{   
    pthread_t th;
    pthread_create(&th, NULL, (void *)&send_broadcast, NULL);
}

int main(int argc, char** argv)
{
    if (argc != 2)
    {
        fprintf(stderr, "usage: %s <uname>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    struct sockaddr_in* in;
    struct ifaddrs* ip_info, *ip_ptr;
    getifaddrs(&ip_info);

    for (ip_ptr = ip_info; ip_ptr != NULL; ip_ptr = ip_ptr->ifa_next)
    {
        if (ip_ptr->ifa_flags & IFF_BROADCAST && ip_ptr->ifa_addr->sa_family == AF_INET)
        {
            if (ip_ptr->ifa_addr)
            {
                in = (struct sockaddr_in *)ip_ptr->ifa_addr;
                inet_ntop(AF_INET, &in->sin_addr, host_ipv4, INET_ADDRSTRLEN);
                printf("getifaddrs IP: %s\n", host_ipv4);
            }
            if (ip_ptr->ifa_broadaddr)
            {
                in = (struct sockaddr_in *)ip_ptr->ifa_broadaddr;
                inet_ntop(AF_INET, &in->sin_addr, broadcast, INET_ADDRSTRLEN);
                printf("getifaddrs Broadcast: %s\n", broadcast);
            }
        }
    }
    freeifaddrs(ip_info);

    memcpy(players[0].uname, argv[1], UNAME_MAX);
    memcpy(players[0].server_ip, host_ipv4, INET_ADDRSTRLEN);

    printf("c - Create Server\n");
    printf("j - Join Server\n");

    char c;
    while (state == SEND_BROADCAST)
    {
        c = getchar();      
        if (c == 'c')
        {
            handle_server();
            handle_broadcast(); 
        }
        else if (c == 'j')
            join_server();

        sleep(100000);
    
        getchar();
    }

    return 0;
}
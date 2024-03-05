#ifndef __x86_64__
    #error "unsupported architecture and bit mode"
#endif

// #if !defined(_WIN32) && (!defined(__linux__) || !defined(__APPLE__))
//     #error "unsupported operating system"
// #endif

// Top macro should be used, but this one
// shows colours for editor :)
#if defined(_WIN32)
    #define WIN_PLATFORM
#elif defined(__linux__) || defined(__APPLE__)
    #define LINUX_PLATFORM
#else
    #error "unsupported operating system"
#endif

#ifdef WIN_PLATFORM
    #define WIN32_LEAN_AND_MEAN
    #include <windows.h>
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #include <iphlpapi.h>
    // #pragma comment(lib, "user32.lib");
    // #pragma comment(lib, "Ws2_32.lib");
#endif
#ifdef LINUX_PLATFORM
    #define _GNU_SOURCE
    #include <unistd.h>
    
    #include <sys/socket.h>
    #include <sys/types.h>
    #include <arpa/inet.h>
    #include <ifaddrs.h>
    #include <net/if.h>

    typedef int SOCKET; // windows uses uint ptr for sockets
    typedef struct sockaddr SOCKADDR;
    typedef struct sockaddr_in SOCKADDR_IN;
    // typedef struct addrinfo* PADDRINFOA;
    typedef struct addrinfo ADDRINFO;

    #define SOCKET_ERROR (-1)
    #define INVALID_SOCKET SOCKET_ERROR
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#define DEBUG
#include "debug.h"

char host_ipv4[INET_ADDRSTRLEN];
char broadcast[INET_ADDRSTRLEN];
char subnet_mask[INET_ADDRSTRLEN];
void get_relevant_addr(void)
{
#ifdef WIN_PLATFORM
    PIP_ADAPTER_ADDRESSES p_adapters = NULL;
    ULONG adapters_sz = 0; 
    ULONG res;
    do {
        res = GetAdaptersAddresses(AF_INET, 0, NULL, p_adapters, &adapters_sz);
        if (res == ERROR_BUFFER_OVERFLOW)
        {
            p_adapters = malloc(adapters_sz);
            if (p_adapters == NULL)
            {
                fprintf(stderr, "Not enough memory to store adapter addresses.\n");
                free(p_adapters);
                exit(EXIT_FAILURE);
            }
            res = GetAdaptersAddresses(AF_INET, 0, NULL, p_adapters, &adapters_sz);
        }
        else
        {
            fprintf(stderr, "Failed to get adapater addresses.\n");
            exit(EXIT_FAILURE);
        }
    } while (res != ERROR_SUCCESS);
    PIP_ADAPTER_ADDRESSES ptr = p_adapters;
    ULONG mask;
    for (; ptr != NULL; ptr = ptr->Next)
    {   
        /* Evidence of an ISP - user has modem */
        // DHCP - Dynamic host configuration protocol
        // provides an IP address, subnet mask and default gateway to DHCP client
        // DDNS - Dynamic domain name service
        // automatically updates IP addresses
        if (ptr->Dhcpv4Enabled && ptr->DdnsEnabled)
        {
            if (ptr->FirstDnsServerAddress)
            {
                PSOCKADDR_IN sock_addr = (PSOCKADDR_IN)ptr->FirstDnsServerAddress->Address.lpSockaddr;
                ConvertLengthToIpv4Mask(ptr->FirstUnicastAddress->OnLinkPrefixLength, &mask);
                ULONG broadcast_addr = (sock_addr->sin_addr.s_addr & INADDR_BROADCAST) | ~mask;

                inet_ntop(AF_INET, &sock_addr->sin_addr, host_ipv4, INET_ADDRSTRLEN);

                inet_ntop(AF_INET, &mask, broadcast, INET_ADDRSTRLEN);

                inet_ntop(AF_INET, &broadcast_addr, subnet_mask, INET_ADDRSTRLEN);
                break;
            }
        }
    }
    free(p_adapters);
#endif
#ifdef LINUX_PLATFORM
    struct sockaddr_in* in;
    struct ifaddrs* ip_info, *ip_ptr;
    getifaddrs(&ip_info);
    
    int flags = IFF_BROADCAST | IFF_UP | IFF_POINTOPOINT | IFF_RUNNING;
    for (ip_ptr = ip_info; ip_ptr != NULL; ip_ptr = ip_ptr->ifa_next)
    {
        if (ip_ptr->ifa_flags & flags && ip_ptr->ifa_addr->sa_family == AF_INET)
        {
            if (ip_ptr->ifa_addr)
            {
                in = (struct sockaddr_in *)ip_ptr->ifa_addr;
                inet_ntop(AF_INET, &in->sin_addr, host_ipv4, INET_ADDRSTRLEN);
            }
            if (ip_ptr->ifa_broadaddr)
            {
                in = (struct sockaddr_in *)ip_ptr->ifa_broadaddr;
                inet_ntop(AF_INET, &in->sin_addr, broadcast, INET_ADDRSTRLEN);
            }
            if (ip_ptr->ifa_netmask)
            {
                in = (struct sockaddr_in *)ip_ptr->ifa_netmask;
                inet_ntop(AF_INET, &in->sin_addr, subnet_mask, INET_ADDRSTRLEN);
            }
        }
    }
    freeifaddrs(ip_info);
#endif
}

int main(void)
{
    get_relevant_addr();
    printf("IP address: %s\n", host_ipv4);
    printf("Subnet: %s\n", subnet_mask);
    printf("Broadcast: %s\n", broadcast);
    return 0;
}

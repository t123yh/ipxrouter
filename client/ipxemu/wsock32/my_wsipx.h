/*
Author: Jelle Geerts

Usage of the works is permitted provided that this instrument is
retained with the works, so that any entity that uses the works is
notified of this instrument.

DISCLAIMER: THE WORKS ARE WITHOUT WARRANTY.
*/

#ifndef MY_WSIPX_H
#define MY_WSIPX_H

#include "compiler_specific.h"
#include <windows.h>

#define NSPROTO_IPX   1000
#define NSPROTO_SPX   1256
#define NSPROTO_SPXII 1257

#define IPX_NODENUM_LEN 6
#define IPX_NETNUM_LEN 4

#pragma pack(push,1)
typedef struct sockaddr_ipx {
    short sa_family;
    char sa_netnum[4];
    char sa_nodenum[6];
    unsigned short sa_socket;
} SOCKADDR_IPX, *PSOCKADDR_IPX, FAR *LPSOCKADDR_IPX;
#pragma pack(pop)

#endif /* !defined(MY_WSIPX_H) */

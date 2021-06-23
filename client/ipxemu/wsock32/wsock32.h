/*
Author: Jelle Geerts

Usage of the works is permitted provided that this instrument is
retained with the works, so that any entity that uses the works is
notified of this instrument.

DISCLAIMER: THE WORKS ARE WITHOUT WARRANTY.
*/

#ifndef WSOCK32_H
#define WSOCK32_H

#include "compiler_specific.h"
#include <winsock2.h>
#include <windows.h>

BOOL WINAPI DllMain(HINSTANCE hInstDll, DWORD fdwReason, LPVOID lpvReserved);

int STDCALL my_bind(SOCKET s, const struct sockaddr *name, int namelen);
int STDCALL my_closesocket(SOCKET s);
int STDCALL my_getsockname(SOCKET s, struct sockaddr *name, int *namelen);
int STDCALL my_getsockopt(SOCKET s, int level, int optname, char *optval,
        int *optlen);
u_long STDCALL my_htonl(u_long hostlong);
u_short STDCALL my_htons(u_short hostshort);
int STDCALL my_recvfrom(SOCKET s, char *buf, int len, int flags,
        struct sockaddr *from, int *fromlen);
int STDCALL my_sendto(SOCKET s, const char *buf, int len, int flags,
        const struct sockaddr *to, int tolen);
int STDCALL my_setsockopt(SOCKET s, int level, int optname, const char *optval,
        int optlen);
SOCKET STDCALL my_socket(int af, int type, int protocol);
void dummy_GetAddressByNameA(void);
void dummy_GetAddressByNameW(void);
int STDCALL my_EnumProtocolsA(LPINT lpiProtocols, LPVOID lpProtocolBuffer,
        LPDWORD lpdwBufferLength);
int STDCALL my_EnumProtocolsW(LPINT lpiProtocols, LPVOID lpProtocolBuffer,
        LPDWORD lpdwBufferLength);
void dummy_SetServiceA(void);
void dummy_SetServiceW(void);
void dummy_GetServiceA(void);
void dummy_GetServiceW(void);

#endif /* !defined(WSOCK32_H) */

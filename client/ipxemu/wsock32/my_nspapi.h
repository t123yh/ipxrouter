/*
Author: Jelle Geerts

Usage of the works is permitted provided that this instrument is
retained with the works, so that any entity that uses the works is
notified of this instrument.

DISCLAIMER: THE WORKS ARE WITHOUT WARRANTY.
*/

#ifndef MY_NSPAPI_H
#define MY_NSPAPI_H

#include "compiler_specific.h"
#include <windows.h>
#pragma pack(push,1)

typedef struct _PROTOCOL_INFOA {
    DWORD dwServiceFlags;
    INT iAddressFamily;
    INT iMaxSockAddr;
    INT iMinSockAddr;
    INT iSocketType;
    INT iProtocol;
    DWORD dwMessageSize;
    LPSTR lpProtocol;
}  PROTOCOL_INFOA;

typedef struct _PROTOCOL_INFOW {
    DWORD dwServiceFlags;
    INT iAddressFamily;
    INT iMaxSockAddr;
    INT iMinSockAddr;
    INT iSocketType;
    INT iProtocol;
    DWORD dwMessageSize;
    LPWSTR lpProtocol;
} PROTOCOL_INFOW;
#pragma pack(pop)


#ifdef UNICODE
typedef PROTOCOL_INFOW PROTOCOL_INFO;
#else /* !defined(UNICODE) */
typedef PROTOCOL_INFOA PROTOCOL_INFO;
#endif /* !defined(UNICODE) */

#endif /* !defined(MY_NSPAPI_H) */

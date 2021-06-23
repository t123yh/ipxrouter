/*
Author: Jelle Geerts

Usage of the works is permitted provided that this instrument is
retained with the works, so that any entity that uses the works is
notified of this instrument.

DISCLAIMER: THE WORKS ARE WITHOUT WARRANTY.
*/

#ifndef MY_WSNWLINK_H
#define MY_WSNWLINK_H

#include "compiler_specific.h"
#include <windows.h>

#define IPX_PTYPE                  0x4000
#define IPX_FILTERPTYPE            0x4001
#define IPX_DSTYPE                 0x4002
#define IPX_STOPFILTERPTYPE        0x4003
#define IPX_EXTENDED_ADDRESS       0x4004
#define IPX_RECVHDR                0x4005
#define IPX_MAXSIZE                0x4006
#define IPX_ADDRESS                0x4007
#define IPX_GETNETINFO             0x4008
#define IPX_GETNETINFO_NORIP       0x4009
#define IPX_SPXGETCONNECTIONSTATUS 0x400B
#define IPX_ADDRESS_NOTIFY         0x400C
#define IPX_MAX_ADAPTER_NUM        0x400D
#define IPX_RERIPNETNUMBER         0x400E
#define IPX_RECEIVE_BROADCAST      0x400F
#define IPX_IMMEDIATESPXACK        0x4010

#pragma pack(push,1)
typedef struct _IPX_ADDRESS_DATA {
    INT adapternum;
    UCHAR netnum[4];
    UCHAR nodenum[6];
    BOOLEAN wan;
    BOOLEAN status;
    INT maxpkt;
    ULONG linkspeed;
} IPX_ADDRESS_DATA, *PIPX_ADDRESS_DATA;
#pragma pack(pop)

#endif /* !defined(MY_WSNWLINK_H) */

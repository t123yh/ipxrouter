/*
Author: Jelle Geerts

Usage of the works is permitted provided that this instrument is
retained with the works, so that any entity that uses the works is
notified of this instrument.

DISCLAIMER: THE WORKS ARE WITHOUT WARRANTY.
*/

#ifndef ENUM_PROTOCOLS_TEMPLATE_H
#define ENUM_PROTOCOLS_TEMPLATE_H

#include <winsock2.h>
#include <windows.h>

void free_protocol_names(void);
int my_EnumProtocolsA_impl(LPINT lpiProtocols, LPVOID lpProtocolBuffer,
        LPDWORD lpdwBufferLength);
int my_EnumProtocolsW_impl(LPINT lpiProtocols, LPVOID lpProtocolBuffer,
        LPDWORD lpdwBufferLength);

#endif /* !defined(ENUM_PROTOCOLS_TEMPLATE_H) */

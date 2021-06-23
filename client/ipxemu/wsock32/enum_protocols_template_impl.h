/*
Author: Jelle Geerts

Usage of the works is permitted provided that this instrument is
retained with the works, so that any entity that uses the works is
notified of this instrument.

DISCLAIMER: THE WORKS ARE WITHOUT WARRANTY.
*/

#include "enum_protocols_template.h"

#include "my_nspapi.h"
#include "my_wsipx.h"
#include <windows.h>
#include <assert.h>
#include <stdlib.h>
#ifdef TEMPLATE_UNICODE
# include <wchar.h>
#else /* !defined(TEMPLATE_UNICODE) */
# include <string.h>
#endif /* !defined(TEMPLATE_UNICODE) */

#undef my_EnumProtocols_impl
#undef _my_EnumProtocols
#undef my_tcsdup
#undef add_protocol_name
#undef protocol_names
#undef protocol_names_size
#undef protocol_names_index
#undef WSAEnumProtocols
#undef PROTOCOL_INFO
#undef LPWSAPROTOCOL_INFO
#undef tcscmp
#undef tcscpy
#undef tcslen
#undef TEXT
#undef tchar

#ifdef TEMPLATE_UNICODE

# define my_EnumProtocols_impl my_EnumProtocolsW_impl
# define _my_EnumProtocols     _my_EnumProtocols_wide
# define my_tcsdup             my_wcsdup
# define add_protocol_name     add_protocol_name_wide
# define protocol_names        protocol_names_wide
# define protocol_names_size   protocol_names_size_wide
# define protocol_names_index  protocol_names_index_wide

# define WSAEnumProtocols   WSAEnumProtocolsW
# define PROTOCOL_INFO      PROTOCOL_INFOW
#define LPWSAPROTOCOL_INFO LPWSAPROTOCOL_INFOW

# define tcscmp wcscmp
# define tcscpy wcscpy
# define tcslen wcslen

# define _TEXT(t) L##t
# define TEXT(t) _TEXT(t)

# define tchar wchar_t

#else /* !defined(TEMPLATE_UNICODE) */

# define my_EnumProtocols_impl my_EnumProtocolsA_impl
# define _my_EnumProtocols     _my_EnumProtocols_ansi
# define my_tcsdup             my_strdup
# define add_protocol_name     add_protocol_name_ansi
# define protocol_names        protocol_names_ansi
# define protocol_names_size   protocol_names_size_ansi
# define protocol_names_index  protocol_names_index_ansi

# define WSAEnumProtocols   WSAEnumProtocolsA
# define PROTOCOL_INFO      PROTOCOL_INFOA
# define LPWSAPROTOCOL_INFO LPWSAPROTOCOL_INFOA

# define tcscmp strcmp
# define tcscpy strcpy
# define tcslen strlen

# define TEXT(t) t

# define tchar char

#endif /* !defined(TEMPLATE_UNICODE) */

static tchar **protocol_names = NULL;

/* Must be at least one so that multiplying by two will not yield zero. */
#define PROTOCOL_NAMES_INITIAL_ARRAY_SIZE 8
static unsigned int protocol_names_size = 0;
static unsigned int protocol_names_index = 0;

static tchar *my_tcsdup(const tchar *s1)
{
    tchar *p;

    if (s1 == NULL)
        return NULL;

    p = (tchar *)malloc(tcslen(s1) * sizeof(s1[0]) + 1);
    if (p != NULL)
        tcscpy(p, s1);

    return p;
}

/* Returns:
 *   On success, the added protocol name.
 *   On failure, a NULL pointer.
 */
static tchar *add_protocol_name(const tchar *protocol_name)
{
    tchar *duplicated_string = NULL;
    unsigned int i;

    if (protocol_names != NULL) {
        for (i = 0; i < protocol_names_index; ++i) {
            if (tcscmp(protocol_names[i], protocol_name) == 0)
                return protocol_names[i];
        }
    }

    if (protocol_names_index >= protocol_names_size) {
        tchar **p;

        i = protocol_names_size ?
            protocol_names_size * 2 :
            PROTOCOL_NAMES_INITIAL_ARRAY_SIZE;
        p = (tchar **)realloc(protocol_names, i * sizeof(protocol_names[0]));
        if (p == NULL) {
            /* Do not yet free `protocol_names', the current elements can still
             * be used to determine whether a given protocol name is already
             * present.
             */
            return NULL;
        }

        protocol_names = p;
        protocol_names_size = i;
    }

    duplicated_string = my_tcsdup(protocol_name);
    if (duplicated_string == NULL)
        return NULL;

    protocol_names[protocol_names_index] = duplicated_string;
    protocol_names_index++;

    return duplicated_string;
}

/*
 * In case of unrecoverable error, `*lpdwBufferLength' is left at the value
 * that was passed, so that one can detect such errors by initializing
 * `*lpdwBufferLength' to an improbable value before calling this function.
 */
static int _my_EnumProtocols(LPINT lpiProtocols, LPVOID lpProtocolBuffer,
        LPDWORD lpdwBufferLength)
{
    int rval = SOCKET_ERROR;
    LPWSAPROTOCOL_INFO protocolBuffer = NULL;
    DWORD bufferLength = 0;
    int i;

    i = WSAEnumProtocols(lpiProtocols, NULL, &bufferLength);
    if (i != SOCKET_ERROR || bufferLength == 0) {
        /* This is reached if and only if there are no protocols installed. */
        *lpdwBufferLength = 0;
        return 0;
    }

    protocolBuffer = (LPWSAPROTOCOL_INFO)malloc(bufferLength);
    if (protocolBuffer == NULL)
        goto done;

    rval = WSAEnumProtocols(lpiProtocols, protocolBuffer, &bufferLength);
    if (rval == SOCKET_ERROR)
        goto done;

    i = rval * sizeof(PROTOCOL_INFO);
    if (*lpdwBufferLength < (DWORD)i) {
        *lpdwBufferLength = i;
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        return SOCKET_ERROR;
    }

    /* Convert the WSAPROTOCOL_INFO structures returned by WSAEnumProtocols()
     * to PROTOCOL_INFO structures.
     */
    for (i = 0; i < rval; ++i) {
        PROTOCOL_INFO *pi = &((PROTOCOL_INFO *)lpProtocolBuffer)[i];

        pi->dwServiceFlags = protocolBuffer[i].dwServiceFlags1;
        pi->iAddressFamily = protocolBuffer[i].iAddressFamily;
        pi->iMaxSockAddr   = protocolBuffer[i].iMaxSockAddr;
        pi->iMinSockAddr   = protocolBuffer[i].iMinSockAddr;
        pi->iSocketType    = protocolBuffer[i].iSocketType;
        pi->iProtocol      = protocolBuffer[i].iProtocol;
        pi->dwMessageSize  = protocolBuffer[i].dwMessageSize;
        pi->lpProtocol     = add_protocol_name(protocolBuffer[i].szProtocol);
    }

done:
    if (protocolBuffer != NULL)
        free(protocolBuffer);
    return rval;
}

int my_EnumProtocols_impl(LPINT lpiProtocols, LPVOID lpProtocolBuffer,
        LPDWORD lpdwBufferLength)
{
    int ipx_requested = 0;
    LPINT lpiProtocols_iter;
    PROTOCOL_INFO *protocolBuffer = (PROTOCOL_INFO *)lpProtocolBuffer;
    int num_protos;
    int have_native_ipx = 0;
    int native_ipx_index = 0;
    int i;

    num_protos = _my_EnumProtocols(lpiProtocols, lpProtocolBuffer,
            lpdwBufferLength);
    if (num_protos == SOCKET_ERROR)
        return SOCKET_ERROR;

    if (lpiProtocols == NULL) {
        /* All protocols requested; that includes IPX. */
        ipx_requested = 1;
    } else {
        lpiProtocols_iter = lpiProtocols;
        for ( ; *lpiProtocols_iter != 0; ++lpiProtocols_iter) {
            if (*lpiProtocols_iter == NSPROTO_IPX) {
                ipx_requested = 1;
                break;
            }
        }
    }

    for (i = 0; i < num_protos; ++i) {
        if (protocolBuffer[i].iAddressFamily == AF_IPX) {
            have_native_ipx = 1;
            native_ipx_index = i;
        }
    }

    if (ipx_requested && !have_native_ipx) {
        DWORD bytes_needed;

        /* Only in this case an extra PROTOCOL_INFO structure is needed, since
         * in case of `ipx_requested && have_native_ipx', the PROTOCOL_INFO
         * structure for native IPX will be replaced.
         */
        num_protos++;

        bytes_needed = num_protos * sizeof(PROTOCOL_INFO);
        if (*lpdwBufferLength < bytes_needed) {
            *lpdwBufferLength = bytes_needed;
            SetLastError(ERROR_INSUFFICIENT_BUFFER);
            return SOCKET_ERROR;
        }
    }

    if (ipx_requested) {
        assert(num_protos > 0);

        i = have_native_ipx ? native_ipx_index : num_protos - 1;

        protocolBuffer[i].dwServiceFlags = XP1_CONNECTIONLESS |
            XP1_MESSAGE_ORIENTED | XP1_SUPPORT_BROADCAST |
            XP1_SUPPORT_MULTIPOINT;
        protocolBuffer[i].iAddressFamily = AF_IPX;
        /* 16 = size of sockaddr_ipx structure padded to 16 bytes. */
        protocolBuffer[i].iMaxSockAddr = 16;
        protocolBuffer[i].iMinSockAddr = sizeof(struct sockaddr_ipx);
        protocolBuffer[i].iSocketType = SOCK_DGRAM;
        protocolBuffer[i].iProtocol = NSPROTO_IPX;
        /* 576 = the maximum message size for some IPX routers. It is the value
         * returned in most (if not all) IPX implementations on Windows. */
        protocolBuffer[i].dwMessageSize = 576;
        protocolBuffer[i].lpProtocol = add_protocol_name(TEXT("IPX"));
    }

    return num_protos;
}

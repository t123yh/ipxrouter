/*
Author: Jelle Geerts

Usage of the works is permitted provided that this instrument is
retained with the works, so that any entity that uses the works is
notified of this instrument.

DISCLAIMER: THE WORKS ARE WITHOUT WARRANTY.
*/

#include "wsock32.h"
#include "enum_protocols_template.h"
#include "my_wsipx.h"
#include "my_wsnwlink.h"
#include "socktable.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ws2tcpip.h>

#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))
#ifdef _DEBUG
#define DEBUG
#endif
#ifdef DEBUG
# include <stdarg.h>
# include <stdio.h>
static FILE* log_fp = NULL;

static void log_close(void)
{
	if (log_fp != NULL) {
		fclose(log_fp);
		log_fp = NULL;
	}
}

static int log_ensure_opened(void)
{
#define LOG_FILENAME "ipxemu-log.txt"
	char filename[2048 + sizeof(LOG_FILENAME)];
	DWORD size = ARRAY_SIZE(filename) -
		(sizeof(LOG_FILENAME) - sizeof(filename[0]));
	DWORD n;

	n = GetTempPath(size, filename);
	if (!n || n > size) {
		assert(0);
		return 0;
	}
	strcat(filename, LOG_FILENAME);

	if (log_fp == NULL)
		log_fp = fopen(LOG_FILENAME, "a");
	return log_fp != NULL;
}

#define log_msg(expr) _log_msg expr
static void _log_msg(const char* fmt, ...)
{
	va_list ap;

	if (!log_ensure_opened())
		return;

	va_start(ap, fmt);
	vfprintf(log_fp, fmt, ap);
	fflush(log_fp);
	va_end(ap);
}

static void log_export_call(const char* export_name)
{
	unsigned int ret = 0xC0FFEE;

	if (!log_ensure_opened())
		return;

	log_msg(("export `%s' called\n", export_name));
	/*
	__asm (
		"mov (%%ebp), %%eax\n"
		"mov 4(%%eax), %%eax\n"
		: "=r" (ret));*/

	log_msg(("return address pushed by caller "
		"module (may be incorrect): %08X\n", ret));

	log_msg(("\n"));
}
#else /* !defined(DEBUG) */
# define log_close() ((void)0)
# define log_msg(expr) ((void)0)
# define log_export_call(a) ((void)0)
#endif /* !defined(DEBUG) */

/* As this function uses FatalAppExit(), it will only return if a debug version
 * of `kernel32.dll' was loaded and the message box was cancelled; otherwise,
 * the function will not return.
 */
static void panic(const char* export_name)
{
	char* error_msg;

	assert(export_name != NULL);

	/* Using snprintf() here would inflate the executable size considerably (as
	 * at the time of writing, this function isn't used elsewhere, and using it
	 * here would cause code providing snprintf() to be linked in).
	 */
	error_msg = malloc(160 + strlen(export_name));
	if (error_msg) {
		strcpy(error_msg, "The exported function `");
		strcat(error_msg, export_name);
		strcat(error_msg,
			"' of `wsock32.dll' was called. However, it is a dummy "
			"(non-functional) function. "
			"The application will be terminated.");
		FatalAppExit(0, error_msg);
		free(error_msg);
	}
}

#ifdef DEBUG
# define INETSTR_MAX 22
/* The output buffer 'buf' must be able to hold at least INETSTR_MAX
 * characters.
 */
static void inetstr(char* buf, const struct sockaddr_in* sa)
{
	unsigned char* port = (unsigned char*)&sa->sin_port;

	sprintf(buf, "%u.%u.%u.%u:%u",
		sa->sin_addr.S_un.S_un_b.s_b1,
		sa->sin_addr.S_un.S_un_b.s_b2,
		sa->sin_addr.S_un.S_un_b.s_b3,
		sa->sin_addr.S_un.S_un_b.s_b4,
		port[0] << 8 | port[1]);
}
#endif /* defined(DEBUG) */

static UCHAR ipx_nodenum[IPX_NODENUM_LEN];
/* Get the 6-byte IPX node number. */
static void get_ipx_nodenum(void* nodenum)
{
	memcpy(nodenum, ipx_nodenum, IPX_NODENUM_LEN);
}

BOOL WINAPI DllMain(HINSTANCE hInstDll, DWORD fdwReason, LPVOID lpvReserved)
{
	(void)hInstDll;
	(void)lpvReserved;

	switch (fdwReason) {
	case DLL_PROCESS_ATTACH:
		srand((unsigned)time(NULL));
		for (int i = 0; i < sizeof(ipx_nodenum); i++) {
			ipx_nodenum[i] = rand();
		}
		log_msg(("DllMain(): DLL_PROCESS_ATTACH\n"));
		return TRUE;

	case DLL_PROCESS_DETACH:
		log_msg(("DllMain(): DLL_PROCESS_DETACH\n"));
		free_protocol_names();
		free_socktable();
		log_close();
		break;

	default:
		break;
	}

	return TRUE;
}

int STDCALL my_bind(SOCKET s, const struct sockaddr* name, int namelen)
{
	const struct sockaddr* sockaddr_to_use = name;
	struct sockaddr_in sa;



	if (is_emulated_socket(s)) {
		const struct sockaddr_ipx* sa_ipx = (const struct sockaddr_ipx*)name;

		if (namelen < (signed)sizeof(struct sockaddr_ipx) ||
			name->sa_family != AF_IPX) {
			WSASetLastError(WSAEFAULT);
			return SOCKET_ERROR;
		}

		struct emulation_options* opt = get_emulation_options(s);

		// memcpy(&opt->local_netnum, sa_ipx->sa_netnum, IPX_NETNUM_LEN);
		if (sa_ipx->sa_socket == 0) {
			opt->local_socket = rand();
		} else {
			opt->local_socket = sa_ipx->sa_socket;
		}

		log_msg(("bind(0x%x): local socket %d(%d), local nodenum %X:%X:%X:%X:%X:%X\n",
			s, opt->local_socket, sa_ipx->sa_socket,
			ipx_nodenum[0], ipx_nodenum[1], ipx_nodenum[2], ipx_nodenum[3], ipx_nodenum[4], ipx_nodenum[5]));

		sa.sin_family = AF_INET;
		sa.sin_port = 0;
		sa.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
		memset(&sa.sin_zero, 0, sizeof(sa.sin_zero));

		sockaddr_to_use = (const struct sockaddr*)&sa;
		namelen = sizeof(sa);
	}

	return bind(s, sockaddr_to_use, namelen);
}

int STDCALL my_closesocket(SOCKET s)
{
	int ret;

	ret = closesocket(s);

	if (ret == 0 && is_emulated_socket(s)) {
		freeaddrinfo(get_emulation_options(s)->server);
		remove_emulated_socket(s);
	}

	return ret;
}

int STDCALL my_getsockname(SOCKET s, struct sockaddr* name, int* namelen)
{
	if (is_emulated_socket(s)) {
		struct sockaddr_in sa;
		int local_fromlen = sizeof(sa);
		struct sockaddr_ipx* sa_ipx = (struct sockaddr_ipx*)name;

		if (*namelen < (signed)sizeof(struct sockaddr_ipx)) {
			WSASetLastError(WSAEFAULT);
			return SOCKET_ERROR;
		}

		if (getsockname(s, (struct sockaddr*)&sa,
			&local_fromlen) == SOCKET_ERROR) {
			return SOCKET_ERROR;
		}

		struct emulation_options* opt = get_emulation_options(s);

		sa_ipx->sa_family = AF_IPX;
		// memcpy(sa_ipx->sa_netnum, opt->local_netnum, IPX_NETNUM_LEN);
		memset(sa_ipx->sa_netnum, 0, IPX_NETNUM_LEN);
		get_ipx_nodenum(sa_ipx->sa_nodenum);
		sa_ipx->sa_socket = opt->local_socket;

		*namelen = sizeof(struct sockaddr_ipx);

		return 0;
	}

	return getsockname(s, name, namelen);
}

int STDCALL my_getsockopt(SOCKET s, int level, int optname, char* optval,
	int* optlen)
{
	if (level == NSPROTO_IPX && is_emulated_socket(s)) {
#define log_unsupported_optname_failure(n)                                    \
        log_msg(("getsockopt(): option %s not "                               \
                    "implemented, returning failure\n", n))
		switch (optname) {
		case IPX_MAX_ADAPTER_NUM:
			*(BOOL*)optval = 1;
			return 0;

		case IPX_ADDRESS: {
			IPX_ADDRESS_DATA* p = (IPX_ADDRESS_DATA*)optval;
			p->adapternum = 0;
			struct emulation_options* opt = get_emulation_options(s);
			// memcpy(p->netnum, opt->local_netnum, IPX_NETNUM_LEN);
			memset(p->netnum, 0, IPX_NETNUM_LEN);
			get_ipx_nodenum(p->nodenum);
			p->wan = FALSE;
			p->status = TRUE;
			/* 1470 = 1500 (maximum transmission unit for Ethernet) -
			 *        30 (IPX packet header size) */
			p->maxpkt = 1400;
			p->linkspeed = 1000000;
			return 0;
		}

		case IPX_PTYPE:
			log_unsupported_optname_failure("IPX_PTYPE");
			return SOCKET_ERROR;
		case IPX_FILTERPTYPE:
			log_unsupported_optname_failure("IPX_FILTERPTYPE");
			return SOCKET_ERROR;
		case IPX_DSTYPE:
			log_unsupported_optname_failure("IPX_DSTYPE");
			return SOCKET_ERROR;
		case IPX_MAXSIZE:
			log_unsupported_optname_failure("IPX_MAXSIZE");
			return SOCKET_ERROR;
		case IPX_GETNETINFO:
			log_unsupported_optname_failure("IPX_GETNETINFO");
			return SOCKET_ERROR;
		case IPX_GETNETINFO_NORIP:
			log_unsupported_optname_failure("IPX_GETNETINFO_NORIP");
			return SOCKET_ERROR;
		case IPX_SPXGETCONNECTIONSTATUS:
			log_unsupported_optname_failure("IPX_SPXGETCONNECTIONSTATUS");
			return SOCKET_ERROR;
		case IPX_ADDRESS_NOTIFY:
			log_unsupported_optname_failure("IPX_ADDRESS_NOTIFY");
			return SOCKET_ERROR;
		case IPX_RERIPNETNUMBER:
			log_unsupported_optname_failure("IPX_RERIPNETNUMBER");
			return SOCKET_ERROR;
		default:
			log_msg(("getsockopt(): option 0x%08X not implemented, "
				"returning failure\n", optname));
			return SOCKET_ERROR;
		}
#undef log_unsupported_optname_failure
	}

	return getsockopt(s, level, optname, optval, optlen);
}

#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))

#ifdef _MSC_VER
#  define PACKED_STRUCT(name) \
    __pragma(pack(push, 1)) struct name __pragma(pack(pop))
#elif defined(__GNUC__)
#  define PACKED_STRUCT(name) struct __attribute__((packed)) name
#endif

PACKED_STRUCT(packet_header) {
	// UCHAR dst_netnum[IPX_NETNUM_LEN];
	UCHAR dst_nodenum[IPX_NODENUM_LEN];
	USHORT dst_socket;
	// UCHAR src_netnum[IPX_NETNUM_LEN];
	UCHAR src_nodenum[IPX_NODENUM_LEN];
	USHORT src_socket;
};

#define PACKET_HEADER_LEN (sizeof(struct packet_header))

int STDCALL my_recvfrom(SOCKET s, char* buf, int len, int flags,
	struct sockaddr* from, int* fromlen)
{

	if (!is_emulated_socket(s) || fromlen == NULL)
		return recvfrom(s, buf, len, flags, from, fromlen);
	struct emulation_options* opt = get_emulation_options(s);
	log_msg(("recvfrom(0x%x): receiving...\n", s));

	struct sockaddr_ipx* sa_ipx = (struct sockaddr_ipx*)from;

	if (*fromlen < (signed)sizeof(struct sockaddr_ipx)) {
		WSASetLastError(WSAEFAULT);
		return SOCKET_ERROR;
	}

	void* recvbuf = malloc(len + PACKET_HEADER_LEN);
	if (!recvbuf) {
		WSASetLastError(WSAENOBUFS);
		return SOCKET_ERROR;
	}
	struct sockaddr_in sa;
	int local_fromlen = sizeof(sa);
	size_t bytes_received = recvfrom(s, recvbuf, len + PACKET_HEADER_LEN, flags,
		(struct sockaddr*)&sa, &local_fromlen);
	log_msg(("recvfrom(0x%x): received %d bytes...\n", s, bytes_received));

	if (bytes_received == SOCKET_ERROR || bytes_received < PACKET_HEADER_LEN) {
		free(recvbuf);
		return SOCKET_ERROR;
	}

	struct packet_header* header = recvbuf;

	if (header->dst_socket != opt->local_socket) {
		log_msg(("Port mismatch: bound to %d, received %d\n", opt->local_socket, header->dst_socket));
		free(recvbuf);
		return SOCKET_ERROR;
	}

	sa_ipx->sa_family = AF_IPX;

	// memcpy(sa_ipx->sa_netnum, header->src_netnum, IPX_NETNUM_LEN);
	memset(sa_ipx->sa_netnum, 0, IPX_NETNUM_LEN);
	memcpy(sa_ipx->sa_nodenum, header->src_nodenum, IPX_NODENUM_LEN);
	sa_ipx->sa_socket = header->src_socket;

	size_t actual_len = MIN(len, bytes_received - PACKET_HEADER_LEN);
	memcpy(buf, ((char*)recvbuf) + PACKET_HEADER_LEN, actual_len);
	free(recvbuf);

	*fromlen = sizeof(struct sockaddr_ipx);

#ifdef DEBUG
	log_msg(("recvfrom(): emulating incoming IPX packet from %d\n",
		*(int*)(sa_ipx->sa_nodenum)));
#endif /* defined(DEBUG) */

	return actual_len;
}

int STDCALL my_sendto(SOCKET s, const char* buf, int len, int flags,
	const struct sockaddr* to, int tolen)
{
	if (!is_emulated_socket(s)) {
		return sendto(s, buf, len, flags, to, tolen);
	}
	if (tolen < (signed)sizeof(struct sockaddr_ipx)) {
		WSASetLastError(WSAEFAULT);
		return SOCKET_ERROR;
	}
	else if (to->sa_family != AF_IPX) {
		WSASetLastError(WSAEAFNOSUPPORT);
		return SOCKET_ERROR;
	}

	struct emulation_options* opt = get_emulation_options(s);

	void* nbuf = malloc(len + PACKET_HEADER_LEN);
	if (!nbuf) {
		WSASetLastError(WSAENOBUFS);
		return SOCKET_ERROR;
	}
	struct packet_header* header = nbuf;

	const struct sockaddr_ipx* sa_ipx = (const struct sockaddr_ipx*)to;

	memcpy(header->dst_nodenum, sa_ipx->sa_nodenum, IPX_NODENUM_LEN);
	// memcpy(header->dst_netnum, sa_ipx->sa_netnum, IPX_NETNUM_LEN);
	header->dst_socket = sa_ipx->sa_socket;
	get_ipx_nodenum(header->src_nodenum);
	// memcpy(header->src_netnum, opt->local_netnum, IPX_NETNUM_LEN);
	header->src_socket = opt->local_socket;

	log_msg(("sendto(): local socket 0x%x emulating outgoing IPX packet to %d\n",
		s, (*(int*)(sa_ipx->sa_nodenum))));

	memcpy((char*)nbuf + PACKET_HEADER_LEN, buf, len);
	int ret = sendto(s, nbuf, len + PACKET_HEADER_LEN, flags, opt->server->ai_addr, opt->server->ai_addrlen);
	free(nbuf);
	return ret;
}

int STDCALL my_setsockopt(SOCKET s, int level, int optname, const char* optval,
	int optlen)
{
	if (level == NSPROTO_IPX) {
#define log_unsupported_optname_success(n)                                    \
        log_msg(("setsockopt(): option %s not "                               \
                    "implemented, returning success\n", n))
		switch (optname) {
		case IPX_PTYPE:
			log_unsupported_optname_success("IPX_PTYPE");
			return 0;
		case IPX_FILTERPTYPE:
			log_unsupported_optname_success("IPX_FILTERPTYPE");
			return 0;
		case IPX_DSTYPE:
			log_unsupported_optname_success("IPX_DSTYPE");
			return 0;
		case IPX_STOPFILTERPTYPE:
			log_unsupported_optname_success("IPX_STOPFILTERPTYPE");
			return 0;
		case IPX_EXTENDED_ADDRESS:
			log_unsupported_optname_success("IPX_EXTENDED_ADDRESS");
			return 0;
		case IPX_RECVHDR:
			log_unsupported_optname_success("IPX_RECVHDR");
			return 0;
		case IPX_MAXSIZE:
			log_unsupported_optname_success("IPX_MAXSIZE");
			return 0;
		case IPX_ADDRESS:
			log_unsupported_optname_success("IPX_ADDRESS");
			return 0;
		case IPX_GETNETINFO:
			log_unsupported_optname_success("IPX_GETNETINFO");
			return 0;
		case IPX_GETNETINFO_NORIP:
			log_unsupported_optname_success("IPX_GETNETINFO_NORIP");
			return 0;
		case IPX_SPXGETCONNECTIONSTATUS:
			log_unsupported_optname_success("IPX_SPXGETCONNECTIONSTATUS");
			return 0;
		case IPX_ADDRESS_NOTIFY:
			log_unsupported_optname_success("IPX_ADDRESS_NOTIFY");
			return 0;
		case IPX_MAX_ADAPTER_NUM:
			log_unsupported_optname_success("IPX_MAX_ADAPTER_NUM");
			return 0;
		case IPX_RERIPNETNUMBER:
			log_unsupported_optname_success("IPX_RERIPNETNUMBER");
			return 0;
		case IPX_RECEIVE_BROADCAST:
			log_unsupported_optname_success("IPX_RECEIVE_BROADCAST");
			return 0;
		case IPX_IMMEDIATESPXACK:
			log_unsupported_optname_success("IPX_IMMEDIATESPXACK");
			return 0;
		default:
			log_msg(("setsockopt(): option 0x%08X not implemented, "
				"returning failure\n", optname));
			return SOCKET_ERROR;
		}
#undef log_unsupported_optname_success
	}

	return setsockopt(s, level, optname, optval, optlen);
}

static void trimstr(char* s) {
	s[strcspn(s, "\n")] = 0;
	s[strcspn(s, " ")] = 0;
	s[strcspn(s, "\r")] = 0;
}

SOCKET STDCALL my_socket(int af, int type, int protocol)
{
	SOCKET s;
	int emulate = 0;

	if (af == AF_IPX && type == SOCK_DGRAM && protocol == NSPROTO_IPX) {
		emulate = 1;

		af = AF_INET;
		type = SOCK_DGRAM;
		protocol = IPPROTO_UDP;
	}

	/* If one creates a datagram UDP socket using WSASocket() without passing
	 * WSA_FLAG_OVERLAPPED in dwFlags, deadlocks can occur in at least one
	 * situation. For example, if one thread is blocking on recvfrom() or
	 * WSARecvFrom(), and another thread calls closesocket(), then both the
	 * call to recvfrom() or WSARecvFrom() and closesocket() will hang.
	 * The bug occurs on:
	 *   - Windows 7 SP1
	 *   - Windows Vista SP2
	 *   - Windows Server 2003 SP2
	 *   - Windows XP x64 SP2
	 *   - Windows XP x86 SP3
	 *   - Windows 2000 SP4
	 *   - Windows NT4 SP6a
	 * The bug does not occur on:
	 *   - Windows 98 SE
	 *   - Windows 95
	 * Note that this does not occur when using socket() instead of
	 * WSASocket().
	 *
	 * The above bug would occur in Dune 2000 in the DirectX DirectPlay
	 * libraries when using WSASocket() here.
	 */
	s = socket(af, type, protocol);

	if (emulate && s != INVALID_SOCKET) {
		if (!add_emulated_socket(s)) {
			closesocket(s);

			WSASetLastError(WSAENOBUFS);
			return SOCKET_ERROR;
		}
		struct emulation_options* opt = get_emulation_options(s);
		FILE* fp;
		fp = fopen("ipxemu-config.txt", "r");
		if (fp == NULL) {
			log_msg(("Failed to open ipxemu-config.txt: %d\n", errno));
			WSASetLastError(WSAEINVAL);
			return SOCKET_ERROR;
		}

		char address[100], port[10];
		fgets(address, sizeof(address) - 1, fp);
		trimstr(address);
		fgets(port, sizeof(port) - 1, fp);
		trimstr(port);
		log_msg(("Config: %s:%s\n", address, port));
		fclose(fp);

		struct addrinfo hints, *res;
		memset(&hints, 0, sizeof(hints));
		hints.ai_family = AF_UNSPEC;
		hints.ai_socktype = SOCK_DGRAM;
		int r;
		if ((r = getaddrinfo(address, port, &hints, &res)) != 0) {
			log_msg(("getaddrinfo failed: %d\n", (r)));
			WSASetLastError(WSAEINVAL);
			return SOCKET_ERROR;
		}
		if (res == NULL) {
			log_msg(("getaddrinfo returned null\n"));
			WSASetLastError(WSAEINVAL);
			return SOCKET_ERROR;
		}
		opt->server = res;

		char server_addr[100] = { 0 };
		DWORD server_addr_size = sizeof(server_addr);
		WSAAddressToStringA(opt->server->ai_addr, opt->server->ai_addrlen, NULL, server_addr, &server_addr_size);
		log_msg(("server address is %s\n", server_addr));
	}

	log_msg(("socket(): s=0x%X (IS%s emulated)\n",
		s, emulate ? "" : " NOT"));

	return s;
}

void dummy_GetAddressByNameA(void)
{
	log_export_call("GetAddressByNameA");
	panic("GetAddressByNameA");
}

void dummy_GetAddressByNameW(void)
{
	log_export_call("GetAddressByNameW");
	panic("GetAddressByNameW");
}

int STDCALL my_EnumProtocolsA(LPINT lpiProtocols, LPVOID lpProtocolBuffer,
	LPDWORD lpdwBufferLength)
{
	return my_EnumProtocolsA_impl(lpiProtocols, lpProtocolBuffer,
		lpdwBufferLength);
}

int STDCALL my_EnumProtocolsW(LPINT lpiProtocols, LPVOID lpProtocolBuffer,
	LPDWORD lpdwBufferLength)
{
	return my_EnumProtocolsW_impl(lpiProtocols, lpProtocolBuffer,
		lpdwBufferLength);
}

void dummy_SetServiceA(void)
{
	log_export_call("SetServiceA");
	panic("SetServiceA");
}

void dummy_SetServiceW(void)
{
	log_export_call("SetServiceW");
	panic("SetServiceW");
}

void dummy_GetServiceA(void)
{
	log_export_call("GetServiceA");
	panic("GetServiceA");
}

void dummy_GetServiceW(void)
{
	log_export_call("GetServiceW");
	panic("GetServiceW");
}

/*
Author: Jelle Geerts

Usage of the works is permitted provided that this instrument is
retained with the works, so that any entity that uses the works is
notified of this instrument.

DISCLAIMER: THE WORKS ARE WITHOUT WARRANTY.
*/

#ifndef SOCKTABLE_H
#define SOCKTABLE_H

#include <winsock2.h>

struct emulation_options {
    struct addrinfo* server;

    USHORT local_socket;
    // char local_netnum[4];
};

void free_socktable(void);
int is_emulated_socket(SOCKET s);
int add_emulated_socket(SOCKET s);
void remove_emulated_socket(SOCKET s);
struct emulation_options* get_emulation_options(SOCKET s);



#endif /* !defined(SOCKTABLE_H) */

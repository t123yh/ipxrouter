/*
Author: Jelle Geerts

Usage of the works is permitted provided that this instrument is
retained with the works, so that any entity that uses the works is
notified of this instrument.

DISCLAIMER: THE WORKS ARE WITHOUT WARRANTY.
*/

#include "enum_protocols_template.h"

#define TEMPLATE_UNICODE
#include "enum_protocols_template_impl.h"

#undef TEMPLATE_UNICODE
#include "enum_protocols_template_impl.h"

void free_protocol_names(void)
{
    unsigned int i;

    if (protocol_names_ansi != NULL) {
        for (i = 0; i < protocol_names_index_ansi; ++i)
            free(protocol_names_ansi[i]);
        free(protocol_names_ansi);
    }

    if (protocol_names_wide != NULL) {
        for (i = 0; i < protocol_names_index_wide; ++i)
            free(protocol_names_wide[i]);
        free(protocol_names_wide);
    }
}

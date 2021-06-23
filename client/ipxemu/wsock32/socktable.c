/*
Author: Jelle Geerts

Usage of the works is permitted provided that this instrument is
retained with the works, so that any entity that uses the works is
notified of this instrument.

DISCLAIMER: THE WORKS ARE WITHOUT WARRANTY.
*/

#include "socktable.h"
#include <assert.h>
#include <stdlib.h>

/* If we intend to store about 256 nodes (the actual number of nodes stored
 * is, on average, probably much lower), then the highest prime below or equal
 * to it is 251.
 */
#define DIVISOR 251

#define HASH(v) ((v) % DIVISOR)

#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))

struct list_list
{
    struct list_node *head;
    struct list_node *tail;
};

struct list_node
{
    SOCKET socket;
    struct emulation_options options;

    struct list_node *next;
};

static struct list_list hash_table[DIVISOR];

static void list_free(struct list_list *list)
{
    struct list_node *node = list->head;

    while (node) {
        struct list_node *f = node;

        node = node->next;
        free(f);
    }

    list->head = list->tail = NULL;
}

static void list_add_node(struct list_list *list, struct list_node *node)
{
    if (!list->head)
        list->head = node;

    if (list->tail)
        list->tail->next = node;
    list->tail = node;
}

static struct list_node *list_remove_node(struct list_list *list, SOCKET s)
{
    struct list_node *before = NULL;
    struct list_node *node;
    int found = 0;

    for (node = list->head; node; node = node->next) {
        if (node->socket == s) {
            found = 1;
            break;
        }

        before = node;
    }

    if (!found) {
        assert(0);
        return NULL;
    }

    if (!before)
        list->head = node->next;
    else
        before->next = node->next;

    if (!node->next)
        list->tail = before;

    return node;
}

static struct list_node *list_search_node(struct list_list *list, SOCKET s)
{
    struct list_node *node;

    for (node = list->head; node; node = node->next) {
        if (node->socket == s)
            return node;
    }

    return NULL;
}

static struct list_node *list_alloc_node(void)
{
    struct list_node *node;

    node = malloc(sizeof(struct list_node));
    if (!node)
        return NULL;

    node->next = NULL;

    return node;
}

static int hash_table_add_node(SOCKET s)
{
    unsigned int hash;
    struct list_node *node;

    hash = HASH(s);

    node = list_alloc_node();
    if (!node)
        return 0;
    node->socket = s;

    list_add_node(&hash_table[hash], node);

    return 1;
}

static void hash_table_remove_node(SOCKET s)
{
    struct list_node *node;

    node = list_remove_node(&hash_table[HASH(s)], s);
    assert(node);
    free(node);
}

static int hash_table_have_node(SOCKET s)
{
    return list_search_node(&hash_table[HASH(s)], s) != NULL;
}

static void hash_table_free(void)
{
    unsigned int i;

    for (i = 0; i < ARRAY_SIZE(hash_table); ++i)
        list_free(&hash_table[i]);
}

void free_socktable(void)
{
    hash_table_free();
}

int is_emulated_socket(SOCKET s)
{
    return hash_table_have_node(s);
}

struct emulation_options* get_emulation_options(SOCKET s) {
    struct list_node* node = list_search_node(&hash_table[HASH(s)], s);
    if (node == NULL) {
        return NULL;
    }
    return &node->options;
}

int add_emulated_socket(SOCKET s)
{
    return hash_table_add_node(s);
}

void remove_emulated_socket(SOCKET s)
{
    hash_table_remove_node(s);
}

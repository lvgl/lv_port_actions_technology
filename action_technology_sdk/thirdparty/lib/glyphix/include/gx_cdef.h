/*
 * This file is part of Glyphix.
 * Copyright (c) 2023-2024, Glyphix Development Team.
 */
#ifndef GX_DM_H_
#define GX_DM_H_

#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * flag definitions in event
 */
#define GX_EVENT_FLAG_AND               0x01            /**< logic and */
#define GX_EVENT_FLAG_OR                0x02            /**< logic or */
#define GX_EVENT_FLAG_CLEAR             0x04            /**< clear flag */


/* glyphix error code definitions */
#define GX_EOK                          0               /**< There is no error */
#define GX_ERROR                        1               /**< A generic error happens */
#define GX_ETIMEOUT                     2               /**< Timed out */
#define GX_EFULL                        3               /**< The resource is full */
#define GX_EEMPTY                       4               /**< The resource is empty */
#define GX_ENOMEM                       5               /**< No memory */
#define GX_ENOSYS                       6               /**< No system */
#define GX_EBUSY                        7               /**< Busy */
#define GX_EIO                          8               /**< IO error */
#define GX_EINTR                        9               /**< Interrupted system call */
#define GX_EINVAL                       10              /**< Invalid argument */

#define GX_WAITING_FOREVER -1

/* Compiler Related Definitions */
#if defined(__ARMCC_VERSION)           /* ARM Compiler */
    #include <stdarg.h>
    #define GX_SECTION(x)               __attribute__((section(x)))
    #define GX_USED                     __attribute__((used))
    #define GX_ALIGN(n)                 __attribute__((aligned(n)))
    #define GX_WEAK                     __attribute__((weak))
    #define GX_INLINE                   static __inline
#elif defined (__IAR_SYSTEMS_ICC__)     /* for IAR Compiler */
    #include <stdarg.h>
    #define GX_SECTION(x)               @ x
    #define GX_USED                     __root
    #define GX_PRAGMA(x)                _Pragma(#x)
    #define GX_ALIGN(n)                 PRAGMA(data_alignment=n)
    #define GX_WEAK                     __weak
    #define GX_INLINE                   static inline
#elif defined (__GNUC__)                /* GNU GCC Compiler */
    #define GX_SECTION(x)               __attribute__((section(x)))
    #define GX_USED                     __attribute__((used))
    #define GX_ALIGN(n)                 __attribute__((aligned(n)))
    #define GX_WEAK                     __attribute__((weak))
    #define GX_INLINE                   static __inline
#elif defined (__ADSPBLACKFIN__)        /* for VisualDSP++ Compiler */
    #include <stdarg.h>
    #define GX_SECTION(x)               __attribute__((section(x)))
    #define GX_USED                     __attribute__((used))
    #define GX_ALIGN(n)                 __attribute__((aligned(n)))
    #define GX_WEAK                     __attribute__((weak))
    #define GX_INLINE                   static inline
#elif defined (_MSC_VER)
    #include <stdarg.h>
    #define GX_SECTION(x)
    #define GX_USED
    #define GX_ALIGN(n)                 __declspec(align(n))
    #define GX_WEAK
    #define GX_INLINE                   static __inline
#elif defined (__TI_COMPILER_VERSION__)
    #include <stdarg.h>
    /* The way that TI compiler set section is different from other(at least
     * GCC and MDK) compilers. See ARM Optimizing C/C++ Compiler 5.9.3 for more
     * details. */
    #define GX_SECTION(x)
    #define GX_USED
    #define GX_PRAGMA(x)                _Pragma(#x)
    #define GX_ALIGN(n)
    #define GX_WEAK
    #define GX_INLINE                   static inline
#elif defined (__TASKING__)
    #include <stdarg.h>
    #define GX_SECTION(x)               __attribute__((section(x)))
    #define GX_USED                     __attribute__((used, protect))
    #define GX_PRAGMA(x)                _Pragma(#x)
    #define GX_ALIGN(n)                 __attribute__((__align(n)))
    #define GX_WEAK                     __attribute__((weak))
    #define GX_INLINE                   static inline
#else
    #error not supported tool chain
#endif

/**
 * @def GX_ALIGN_UPON(size, align)
 * Return the most contiguous size aligned at specified width. GX_ALIGN_UPON(13, 4)
 * would return 16.
 */
#define GX_ALIGN_UPON(size, align)      (((size) + (align) - 1) & ~((align) - 1))

/**
 * @def GX_ALIGN_DOWN(size, align)
 * Return the down number of aligned at specified width. GX_ALIGN_DOWN(13, 4)
 * would return 12.
 */
#define GX_ALIGN_DOWN(size, align)      ((size) & ~((align) - 1))

/**
 * double list structure
 */
struct gx_list_node
{
    struct gx_list_node *next;                          /**< point to next node. */
    struct gx_list_node *prev;                          /**< point to prev node. */
};
typedef struct gx_list_node gx_list_t;                  /**< type for lists. */

/**
 * single list structure
 */
struct gx_slist_node
{
    struct gx_slist_node *next;                         /**< point to next node. */
};
typedef struct gx_slist_node gx_slist_t;                /**< type for single list. */

/**
 * gx_container_of - return the member address of ptr, if the type of ptr is the
 * struct type.
 */
#define gx_container_of(ptr, type, member) \
    ((type *)((char *)(ptr) - (uintptr_t)(&((type *)0)->member)))

/**
 * @brief initialize a list object
 */
#define GX_LIST_OBJECT_INIT(object) { &(object), &(object) }

/**
 * @brief initialize a list
 *
 * @param l list to be initialized
 */
GX_INLINE void gx_list_init(gx_list_t *l)
{
    l->next = l->prev = l;
}

/**
 * @brief insert a node after a list
 *
 * @param l list to insert it
 * @param n new node to be inserted
 */
GX_INLINE void gx_list_insert_after(gx_list_t *l, gx_list_t *n)
{
    l->next->prev = n;
    n->next = l->next;

    l->next = n;
    n->prev = l;
}

/**
 * @brief insert a node before a list
 *
 * @param n new node to be inserted
 * @param l list to insert it
 */
GX_INLINE void gx_list_insert_before(gx_list_t *l, gx_list_t *n)
{
    l->prev->next = n;
    n->prev = l->prev;

    l->prev = n;
    n->next = l;
}

/**
 * @brief remove node from list.
 * @param n the node to remove from the list.
 */
GX_INLINE void gx_list_remove(gx_list_t *n)
{
    n->next->prev = n->prev;
    n->prev->next = n->next;

    n->next = n->prev = n;
}

/**
 * @brief tests whether a list is empty
 * @param l the list to test.
 */
GX_INLINE int gx_list_isempty(const gx_list_t *l)
{
    return l->next == l;
}

/**
 * @brief get the list length
 * @param l the list to get.
 */
GX_INLINE unsigned int gx_list_len(const gx_list_t *l)
{
    unsigned int len = 0;
    const gx_list_t *p = l;
    while (p->next != l)
    {
        p = p->next;
        len ++;
    }

    return len;
}

/**
 * @brief get the struct for this entry
 * @param node the entry point
 * @param type the type of structure
 * @param member the name of list in structure
 */
#define gx_list_entry(node, type, member) \
    gx_container_of(node, type, member)

/**
 * gx_list_for_each - iterate over a list
 * @pos:    the gx_list_t * to use as a loop cursor.
 * @head:   the head for your list.
 */
#define gx_list_for_each(pos, head) \
    for (pos = (head)->next; pos != (head); pos = pos->next)

/**
 * gx_list_for_each_safe - iterate over a list safe against removal of list entry
 * @pos:    the gx_list_t * to use as a loop cursor.
 * @n:      another gx_list_t * to use as temporary storage
 * @head:   the head for your list.
 */
#define gx_list_for_each_safe(pos, n, head) \
    for (pos = (head)->next, n = pos->next; pos != (head); \
        pos = n, n = pos->next)

/**
 * gx_list_for_each_entry  -   iterate over list of given type
 * @pos:    the type * to use as a loop cursor.
 * @head:   the head for your list.
 * @member: the name of the list_struct within the struct.
 */
#define gx_list_for_each_entry(pos, head, member) \
    for (pos = gx_list_entry((head)->next, typeof(*pos), member); \
         &pos->member != (head); \
         pos = gx_list_entry(pos->member.next, typeof(*pos), member))

/**
 * gx_list_for_each_entry_safe - iterate over list of given type safe against removal of list entry
 * @pos:    the type * to use as a loop cursor.
 * @n:      another type * to use as temporary storage
 * @head:   the head for your list.
 * @member: the name of the list_struct within the struct.
 */
#define gx_list_for_each_entry_safe(pos, n, head, member) \
    for (pos = gx_list_entry((head)->next, typeof(*pos), member), \
         n = gx_list_entry(pos->member.next, typeof(*pos), member); \
         &pos->member != (head); \
         pos = n, n = gx_list_entry(n->member.next, typeof(*n), member))

/**
 * gx_list_first_entry - get the first element from a list
 * @ptr:    the list head to take the element from.
 * @type:   the type of the struct this is embedded in.
 * @member: the name of the list_struct within the struct.
 *
 * Note, that list is expected to be not empty.
 */
#define gx_list_first_entry(ptr, type, member) \
    gx_list_entry((ptr)->next, type, member)

#define GX_SLIST_OBJECT_INIT(object) { NULL }

/**
 * @brief initialize a single list
 *
 * @param l the single list to be initialized
 */
GX_INLINE void gx_slist_init(gx_slist_t *l)
{
    l->next = NULL;
}

GX_INLINE void gx_slist_append(gx_slist_t *l, gx_slist_t *n)
{
    struct gx_slist_node *node = NULL;

    node = l;
    while (node->next) node = node->next;

    /* append the node to the tail */
    node->next = n;
    n->next = NULL;
}

GX_INLINE void gx_slist_insert(gx_slist_t *l, gx_slist_t *n)
{
    n->next = l->next;
    l->next = n;
}

GX_INLINE unsigned int gx_slist_len(const gx_slist_t *l)
{
    unsigned int len = 0;
    const gx_slist_t *list = l->next;
    while (list != NULL)
    {
        list = list->next;
        len ++;
    }

    return len;
}

GX_INLINE gx_slist_t *gx_slist_remove(gx_slist_t *l, gx_slist_t *n)
{
    /* remove slist head */
    struct gx_slist_node *node = l;
    while (node->next && node->next != n) node = node->next;

    /* remove node */
    if (node->next != (gx_slist_t *)0) node->next = node->next->next;

    return l;
}

GX_INLINE gx_slist_t *gx_slist_first(gx_slist_t *l)
{
    return l->next;
}

GX_INLINE gx_slist_t *gx_slist_tail(gx_slist_t *l)
{
    while (l->next) l = l->next;

    return l;
}

GX_INLINE gx_slist_t *gx_slist_next(gx_slist_t *n)
{
    return n->next;
}

GX_INLINE int gx_slist_isempty(gx_slist_t *l)
{
    return l->next == NULL;
}

/**
 * @brief get the struct for this single list node
 * @param node the entry point
 * @param type the type of structure
 * @param member the name of list in structure
 */
#define gx_slist_entry(node, type, member) \
    gx_container_of(node, type, member)

/**
 * gx_slist_for_each - iterate over a single list
 * @pos:    the gx_slist_t * to use as a loop cursor.
 * @head:   the head for your single list.
 */
#define gx_slist_for_each(pos, head) \
    for (pos = (head)->next; pos != NULL; pos = pos->next)

/**
 * gx_slist_for_each_entry  -   iterate over single list of given type
 * @pos:    the type * to use as a loop cursor.
 * @head:   the head for your single list.
 * @member: the name of the list_struct within the struct.
 */
#define gx_slist_for_each_entry(pos, head, member) \
    for (pos = gx_slist_entry((head)->next, typeof(*pos), member); \
         &pos->member != (NULL); \
         pos = gx_slist_entry(pos->member.next, typeof(*pos), member))

/**
 * gx_slist_first_entry - get the first element from a slist
 * @ptr:    the slist head to take the element from.
 * @type:   the type of the struct this is embedded in.
 * @member: the name of the slist_struct within the struct.
 *
 * Note, that slist is expected to be not empty.
 */
#define gx_slist_first_entry(ptr, type, member) \
    gx_slist_entry((ptr)->next, type, member)

/**
 * gx_slist_tail_entry - get the tail element from a slist
 * @ptr:    the slist head to take the element from.
 * @type:   the type of the struct this is embedded in.
 * @member: the name of the slist_struct within the struct.
 *
 * Note, that slist is expected to be not empty.
 */
#define gx_slist_tail_entry(ptr, type, member) \
    gx_slist_entry(gx_slist_tail(ptr), type, member)

#ifdef __cplusplus
}
#endif

#endif

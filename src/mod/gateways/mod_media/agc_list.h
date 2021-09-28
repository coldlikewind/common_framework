
#ifndef __AGC_LIST_H__
#define __AGC_LIST_H__

#include "agc.h"

AGC_BEGIN_EXTERN_C

typedef struct agc_node agc_list_t;
typedef struct agc_node agc_node_t;

struct agc_node {
    agc_node_t *prev;
    agc_node_t *next;
};

#define agc_list_init(plist_) \
do {\
    (plist_)->prev = NULL; \
    (plist_)->next = NULL; \
} while (0)

#define agc_list_is_empty(plist_) ((plist_)->next == NULL)

#define agc_list_first(plist_) ((void*)((plist_)->next))

#define agc_list_last(plist_) ((void*)((plist_)->prev))

#define agc_list_prev(pnode_) ((void*)(((agc_node_t *)(pnode_))->prev))

#define agc_list_next(pnode_) ((void*)(((agc_node_t *)(pnode_))->next))

#define agc_list_prepend(plist_, __ptr_new) \
 do { \
    ((agc_node_t*)(__ptr_new))->prev = NULL; \
    ((agc_node_t*)(__ptr_new))->next = (plist_)->next; \
    if ((plist_)->next) \
        ((plist_)->next)->prev = (agc_node_t*)(__ptr_new); \
    else \
        (plist_)->prev = (agc_node_t*)(__ptr_new); \
    (plist_)->next = (agc_node_t*)(__ptr_new); \
} while (0)

#define agc_list_append(plist_, __ptr_new) \
 do { \
    ((agc_node_t*)(__ptr_new))->prev = (plist_)->prev; \
    ((agc_node_t*)(__ptr_new))->next = NULL; \
    if ((plist_)->prev) \
        ((plist_)->prev)->next = (agc_node_t*)(__ptr_new); \
    else \
        (plist_)->next = (agc_node_t*)(__ptr_new); \
    ((plist_)->prev) = (agc_node_t*)(__ptr_new); \
} while (0)

#define agc_list_insert_prev(plist_, pnode_, __ptr_new) \
do { \
    ((agc_node_t*)(__ptr_new))->prev = ((agc_node_t*)(pnode_))->prev; \
    ((agc_node_t*)(__ptr_new))->next = (agc_node_t*)(pnode_); \
    if (((agc_node_t*)(pnode_))->prev) \
        ((agc_node_t*)(pnode_))->prev->next = (agc_node_t*)(__ptr_new); \
    else \
        (plist_)->next = (agc_node_t*)(__ptr_new); \
    ((agc_node_t*)(pnode_))->prev = ((agc_node_t*)(__ptr_new)); \
} while (0)

#define agc_list_insert_next(plist_, pnode_, __ptr_new) do { \
    ((agc_node_t*)(__ptr_new))->prev = (agc_node_t*)(pnode_); \
    ((agc_node_t*)(__ptr_new))->next = ((agc_node_t*)(pnode_))->next; \
    if (((agc_node_t*)(pnode_))->next) \
        ((agc_node_t*)(pnode_))->next->prev = (agc_node_t*)(__ptr_new); \
    else \
        (plist_)->prev = (agc_node_t*)(__ptr_new); \
    ((agc_node_t*)(pnode_))->next = ((agc_node_t*)(__ptr_new)); \
} while (0)

#define agc_list_remove(plist_, pnode_) do { \
    agc_node_t *_iter = (plist_)->next; \
    while (_iter) { \
        if (_iter == (agc_node_t*)(pnode_)) { \
            if (_iter->prev) \
                _iter->prev->next = _iter->next; \
            else \
                (plist_)->next = _iter->next; \
            if (_iter->next) \
                _iter->next->prev = _iter->prev; \
            else \
                (plist_)->prev = _iter->prev; \
            break; \
        } \
        _iter = _iter->next; \
    } \
} while (0)

typedef int (*ln_cmp_cb)(agc_node_t *pnode1, agc_node_t *pnode2);

#define agc_list_insert_sorted(plist_, __ptr_new, __cmp_callback) do { \
    ln_cmp_cb __pcb = (ln_cmp_cb)__cmp_callback; \
    agc_node_t *_iter = agc_list_first(plist_); \
    while (_iter) { \
        if ((*__pcb)((agc_node_t*)(__ptr_new), _iter) < 0) { \
            agc_list_insert_prev(plist_, _iter, __ptr_new); \
            break; \
        } \
        _iter = agc_list_next(_iter); \
    } \
    if (_iter == NULL) \
        agc_list_append(plist_, __ptr_new); \
} while (0)

AGC_END_EXTERN_C

#endif /* ! __AGC_LIST_H__ */

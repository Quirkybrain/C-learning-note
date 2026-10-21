/**
 * @file dlist.c
 * @author quirkybrain
 * @brief 双向链表的创建入口
 * @version 0.1
 * @date 2026-10-21
 */

#include <stdlib.h>
#include "dlist.h"


/**
 * @brief 双向链表操作接口实现
 */
static const ListVtbl DLIST_VTBL = {
        .get        = dlist_get,
        .index_of   = dlist_index_of,
        .push_front = dlist_push_front,
        .push_back  = dlist_push_back,
        .remove     = dlist_remove,
        .pop_front  = dlist_pop_front,
        .pop_back   = dlist_pop_back,
        .destroy    = dlist_destroy
};

/** @brief 创建并初始化双向链表 */
List* dlist_create (
        size_t elem_size,
        const ListTypeOps* ops
) {
        if (elem_size == 0 || !ops) return NULL;

        DList* dlist = calloc(1, sizeof(DList));
        if (!dlist) return NULL;

        dlist->base.elem_size = elem_size;
        dlist->base.size = 0;
        dlist->base.vtbl = &DLIST_VTBL;
        dlist->base.ops = ops;

        dlist->head = NULL;
        dlist->tail = NULL;

        return &dlist->base;
}
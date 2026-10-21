/**
 * @file slist.c
 * @author quirkybrain
 * @brief 单链表的创建和弹出操作
 * @version 0.1
 * @date 2026-10-21
 */

#include <stdlib.h>
#include "slist.h"
#include "container_of.h"


/**
 * @brief 单链表操作接口实现
 */
static const ListVtbl SLIST_VTBL = {
        .get        = slist_get,
        .index_of   = slist_index_of,
        .push_front = slist_push_front,
        .push_back  = slist_push_back,
        .remove     = slist_remove,
        .pop_front  = slist_pop_front,
        .pop_back   = slist_pop_back,
        .destroy    = slist_destroy
};

/** @brief 创建并初始化单链表 */
List* slist_create (
        size_t elem_size,
        const ListTypeOps* ops
) {
        if (elem_size == 0 || !ops) return NULL;

        SList* slist = calloc(1, sizeof(SList));
        if (!slist) return NULL;
        
        slist->base.elem_size = elem_size;
        slist->base.size = 0;
        slist->base.vtbl = &SLIST_VTBL;
        slist->base.ops = ops;

        slist->head = NULL;
        slist->tail = NULL;

        return &slist->base;
}

/** @brief 弹出首元素并返回其数据指针 */
void* slist_pop_front(List* self) {
        if (!self) return NULL;

        SList* slist = container_of(
                self,
                SList,
                base
        );

        if (
                !slist->head ||
                !slist->tail
        ) return NULL;


        SNode* first = slist->head;
        void* result = first->data;

        if (slist->head == slist->tail) {
                slist->head = NULL;
                slist->tail = NULL;
        } else {
                slist->head = slist->head->next;
        }

        free(first);

        self->size--;
        
        return result;
}

/** @brief 弹出尾元素并返回其数据指针 */
void* slist_pop_back(List* self) {
        if (!self) return NULL;

        SList* slist = container_of(
                self,
                SList,
                base
        );

        if (
                !slist->head ||
                !slist->tail
        ) return NULL;


        SNode* last = slist->tail;
        void* result = last->data;

        if (slist->head == slist->tail) {
                slist->head = NULL;
                slist->tail = NULL;
        } else {
                SNode* prev = slist->head;

                while (prev->next != last) {
                        prev = prev->next;
                }

                prev->next = NULL;
                slist->tail = prev;
        }

        free(last);

        self->size--;

        return result;
}
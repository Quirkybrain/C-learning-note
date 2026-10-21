/**
 * @file list.c
 * @author quirkybrain
 * @brief 通用链表的创建入口
 * @version 0.1
 * @date 2026-10-21
 */

#include "list.h"
#include "slist.h"
#include "dlist.h"

/** @brief 根据链表结构调用对应的创建函数 */
List* createList (
        ListKind kind,
        size_t elem_size,
        const ListTypeOps* ops
) {
        /* 检查元素大小和必需的类型操作 */
        if (
                elem_size == 0 ||
                !ops ||
                !ops->copy ||
                !ops->equals
        ) return NULL;

        /* 根据类型选择链表实现 */
        switch (kind) {
        case SINGLY:
                return slist_create(elem_size, ops);
        case DOUBLY:
                return dlist_create(elem_size, ops);
        case CIRCULAR:
        case CIRCULAR_DOUBLY:
        default:
                return NULL;
        }
};
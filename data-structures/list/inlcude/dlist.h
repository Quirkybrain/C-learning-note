/**
 * @file dlist.h
 * @author quirkybrain
 * @brief 双向链表的节点、结构和操作声明
 * @version 0.1
 * @date 2026-10-21
 */

#ifndef D_LIST_H_
#define D_LIST_H_

#include "list.h"


/**
 * @brief 节点保存元素数据和相邻节点的位置
 */
typedef struct DNode {
        void*         data; /**< 元素数据 */
        struct DNode* next; /**< 下一个节点 */
        struct DNode* prev; /**< 上一个节点 */
} DNode;

/**
 * @brief 双向链表在通用接口之外保存头尾节点
 */
typedef struct DList {
        List   base; /**< 通用链表接口 */
        DNode* head; /**< 头节点 */
        DNode* tail; /**< 尾节点 */
} DList;

/**
 * @brief 弹出双向链表首元素
 */
void*       dlist_pop_front  (List* self);
/** @brief 弹出双向链表尾元素 */
void*       dlist_pop_back   (List* self);
/** @brief 销毁双向链表 */
int         dlist_destroy    (List* self);
/** @brief 在双向链表表头插入元素 */
int         dlist_push_front (List* self, const void* elem);
/** @brief 在双向链表表尾插入元素 */
int         dlist_push_back  (List* self, const void* elem);
/** @brief 删除指定位置的元素 */
int         dlist_remove     (List* self, const size_t index);
/** @brief 获取指定位置的元素 */
const void* dlist_get        (const List* self, const size_t index);
/** @brief 查找元素位置 */
size_t      dlist_index_of   (const List* self, const void* target);

/**
 * @brief 创建双向链表
 * @param elem_size 单个元素的字节数
 * @param ops 元素类型操作表
 * @return 创建的链表，失败时返回 NULL
 */
List* dlist_create (size_t elem_size, const ListTypeOps* ops);

#endif
/**
 * @file list.h
 * @author quirkybrain
 * @brief 链表的通用接口与类型定义
 * @version 0.1
 * @date 2026-10-20
 */
#ifndef LIST_H_
#define LIST_H_

#include <stddef.h>

typedef struct List List;
typedef struct ListVtbl ListVtbl;

/**
 * @brief 对象类型提供的比较、复制和清理操作
 */
typedef struct ListTypeOps {
        int         (*equals)       (const void* a, const void* b); /**< 比较两个元素 */
        int         (*copy)         (void* dst, const void* src); /**< 复制元素 */
        void        (*drop)         (void* value); /**< 清理元素 */
} ListTypeOps;

/**
 * @brief List 操作接口
 */
typedef struct ListVtbl {
        void*       (*pop_front)  (List* self); /**< 弹出首元素 */
        void*       (*pop_back)   (List* self); /**< 弹出尾元素 */
        int         (*destroy)    (List* self); /**< 销毁链表 */
        int         (*push_front) (List* self, const void* elem); /**< 在表头插入元素 */
        int         (*push_back)  (List* self, const void* elem); /**< 在表尾插入元素 */
        int         (*remove)     (List* self, const size_t index); /**< 删除指定位置的元素 */
        const void* (*get)        (const List* self, const size_t index); /**< 获取指定位置的元素 */
        size_t      (*index_of)   (const List* self, const void* target); /**< 查找元素位置 */
} ListVtbl;

/**
 * @brief List 实现的公共部分
 */
typedef struct List {
        size_t             elem_size; /**< 单个元素的字节数 */
        size_t             size;      /**< 当前元素个数 */
        const ListVtbl*    vtbl;      /**< 链表操作接口 */
        const ListTypeOps* ops;       /**< 元素类型操作表 */
} List;

/**
 * @brief 可选择的 List 结构
 */
typedef enum {
        SINGLY,         /**< 单链表 */
        DOUBLY,         /**< 双向链表 */
        CIRCULAR,       /**< 循环链表 */
        CIRCULAR_DOUBLY /**< 循环双链表 */
} ListKind;

/**
 * @brief 按指定结构和元素类型创建 List
 * @param kind 链表结构
 * @param elem_size 单个元素的字节数
 * @param ops 元素类型操作表
 * @return 创建的链表，失败时返回 NULL
 */
List* createList (
        ListKind kind,
        size_t elem_size,
        const ListTypeOps* ops
);

#endif
/**
 * @file slist.c
 * @author quirkybrain
 * @brief 单链表操作
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

/** @brief 节点释放 */
static inline void slist_node_free(
        SList* self,
        SNode* prev,
        SNode* current,
        SNode* next
) {
        /* 从链表摘除节点，同时维护头尾指针。 */
        if (prev) {
                prev->next = next;
        } else {
                self->head = next;
        }

        if (!next) {
                self->tail = prev;
        }

        /* 先清理元素内部资源，再释放元素存储和节点。 */
        if (self->base.ops && self->base.ops->drop) {
                self->base.ops->drop(current->data);
        }

        free(current->data);
        free(current);
        self->base.size--;
}

/** @brief 创建并初始化单链表 */
List* slist_create(
        size_t elem_size,
        const ListTypeOps* ops
) {
        if (
                elem_size == 0 ||
                !ops ||
                !ops->copy ||
                !ops->equals
        ) return NULL;

        SList* slist = calloc(1, sizeof(SList));
        if (!slist) return NULL;

        /* 保存回调表副本，使传入的 ops 不必与链表同寿命。 */
        ListTypeOps* ops_copy = calloc(1, sizeof(ListTypeOps));
        if (!ops_copy) {
                free(slist);
                return NULL;
        }

        *(ops_copy) = (ListTypeOps){
                .copy = ops->copy,
                .drop = ops->drop,
                .equals = ops->equals
        };

        slist->base.elem_size = elem_size;
        slist->base.size = 0;
        slist->base.vtbl = &SLIST_VTBL;
        slist->base.ops = ops_copy;

        slist->head = NULL;
        slist->tail = NULL;

        return &slist->base;
}

/** @brief 弹出首元素并返回其数据指针 */
void* slist_pop_front(List* self) {
        if (!self) return NULL;

        SList* slist = container_of(self, SList, base);

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

        /* 只释放节点，元素数据交给调用方。 */
        free(first);

        self->size--;

        return result;
}

/** @brief 弹出尾元素并返回其数据指针 */
void* slist_pop_back(List* self) {
        if (!self) return NULL;

        SList* slist = container_of(self, SList, base);

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
                /* 单链表没有前驱指针，需要从头查找尾节点的前驱。 */
                SNode* prev = slist->head;

                while (prev->next != last) {
                        prev = prev->next;
                }

                prev->next = NULL;
                slist->tail = prev;
        }

        /* 只释放节点，元素数据交给调用方。 */
        free(last);

        self->size--;

        return result;
}

/** @brief 添加首元素 */
int slist_push_front(List* self, const void* elem) {
        if (!self || !elem) return 0;

        SList* slist = container_of(self, SList, base);

        SNode* new_node = calloc(1, sizeof(SNode));
        if (!new_node) return 0;

        void* result_copy = malloc(self->elem_size);
        if (!result_copy) {
                free(new_node);
                return 0;
        }

        /* 复制失败时，copy 负责回收其已分配的内部资源。 */
        if (!self->ops->copy(result_copy, elem)) {
                free(result_copy);
                free(new_node);
                return 0;
        }

        new_node->data = result_copy;
        new_node->next = slist->head;
        slist->head = new_node;

        if (!slist->tail) {
                slist->tail = new_node;
        }

        self->size++;

        return 1;
}

/** @brief 添加尾元素 */
int slist_push_back(List* self, const void* elem) {
        if (!self || !elem) return 0;

        SList* slist = container_of(self, SList, base);

        SNode* new_node = calloc(1, sizeof(SNode));
        if (!new_node) return 0;

        void* result_copy = malloc(self->elem_size);
        if (!result_copy) {
                free(new_node);
                return 0;
        }

        /* 复制失败时，copy 负责回收其已分配的内部资源。 */
        if (!self->ops->copy(result_copy, elem)) {
                free(result_copy);
                free(new_node);
                return 0;
        }

        new_node->data = result_copy;
        new_node->next = NULL;

        if (slist->tail) {
                slist->tail->next = new_node;
        } else {
                slist->head = new_node;
        }

        slist->tail = new_node;

        self->size++;

        return 1;
}

/** @brief 删除指定位置的节点及其元素 */
int slist_remove(List* self, const size_t index) {
        if (!self || index >= self->size) return 0;

        SList* slist = container_of(self, SList, base);

        SNode* prev = NULL;
        SNode* current = slist->head;

        for (size_t i = 0; i < index; i++) {
                prev = current;
                current = current->next;
        }

        slist_node_free(slist, prev, current, current->next);

        return 1;
}

/** @brief 清理全部节点并释放链表 */
int slist_destroy(List* self) {
        if (!self) return 0;

        SList* slist = container_of(self, SList, base);

        SNode* current = NULL;

        while (self->size != 0) {
                current = slist->head;
                slist_node_free(
                        slist,
                        NULL,
                        current,
                        current->next
                );
        }

        /* 所有元素清理完成后再释放创建时保存的操作表副本。 */
        free((void*)slist->base.ops);

        free(slist);
        return 1;
}

/** @brief 返回指定元素的只读借用指针 */
const void* slist_get(const List* self, const size_t index) {
        if (!self || index >= self->size) return NULL;

        SNode* result = container_of(self, SList, base)->head;

        for (size_t i = 0; i < index; i++, result = result->next);

        return (const void*)result->data;
}

/** @brief 查找首个相等元素并输出索引 */
int slist_index_of(const List* self, size_t* index, const void* target) {
        if (!self || !index || !target) return 0;

        SNode* result = container_of(self, SList, base)->head;

        size_t elem_nums = self->size;

        for (size_t i = 0; i < elem_nums; i++, result = result->next) {
                if (self->ops->equals(target, result->data)) {
                        *index = i;
                        return 1;
                }
        }

        return 0;
}

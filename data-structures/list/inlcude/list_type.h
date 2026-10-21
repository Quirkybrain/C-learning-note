/**
 * @file list_type.h
 * @author quirkybrain
 * @brief 为指定元素类型生成链表包装接口
 * @version 0.1
 * @date 2026-10-20
 */

#ifndef LIST_TYPE_H_
#define LIST_TYPE_H_

#include "list.h"

/**
 * @brief 为元素类型生成链表包装类型和操作函数
 * @param T 元素类型
 * @param Suffix 生成的类型及函数名后缀
 * @param EqualsFn 元素比较函数
 * @param CopyFn 元素复制函数
 * @param DropFn 元素清理函数
 */
#define LIST_TYPE(T, Suffix, EqualsFn, CopyFn, DropFn) \
\
typedef struct {\
        List* impl;\
} List_##Suffix;\
\
static const ListTypeOps listTypeOps_##Suffix = {\
        .equals = (EqualsFn),\
        .copy   = (CopyFn),\
        .drop   = (DropFn)\
};\
\
static inline List_##Suffix List_##Suffix##_new (\
        ListKind kind\
) {\
        List_##Suffix result = {\
                .impl = createList (\
                        kind,\
                        sizeof(T),\
                        &listTypeOps_##Suffix\
                )\
        };\
        return result;\
}\
\
static inline int List_##Suffix##_push_front (\
        List_##Suffix* self,\
        const T* elem\
) {\
        return self && self->impl && self->impl->vtbl && self->impl->vtbl->push_front(self->impl, elem);\
}\
\
static inline int List_##Suffix##_push_back (\
        List_##Suffix* self,\
        const T* elem\
) {\
        return self && self->impl && self->impl->vtbl && self->impl->vtbl->push_back(self->impl, elem);\
}\
\
static inline T* List_##Suffix##_pop_front (\
        List_##Suffix* self\
) {\
        if (self && self->impl && self->impl->vtbl)\
                return (T*)self->impl->vtbl->pop_front(self->impl);\
        return NULL;\
}\
\
static inline T* List_##Suffix##_pop_back (\
        List_##Suffix* self\
) {\
        if (self && self->impl && self->impl->vtbl)\
                return (T*)self->impl->vtbl->pop_back(self->impl);\
        return NULL;\
}\
\
static inline int List_##Suffix##_remove (\
        List_##Suffix* self,\
        const size_t index\
) {\
        return self && self->impl && self->impl->vtbl && self->impl->vtbl->remove(self->impl, index);\
}\
\
static inline int List_##Suffix##_destroy (\
        List_##Suffix* self\
) {\
        if (\
                !self ||\
                !self->impl ||\
                !self->impl->vtbl ||\
                !self->impl->vtbl->destroy\
        ) return 0;\
\
        int ok = self->impl->vtbl->destroy(self->impl);\
        if (ok) self->impl = NULL;\
        return ok;\
}\
\
static inline const T* List_##Suffix##_get (\
        const List_##Suffix* self,\
        const size_t index\
) {\
        if (self && self->impl && self->impl->vtbl)\
                return (const T*)self->impl->vtbl->get(self->impl, index);\
        return NULL;\
}\
\
static inline size_t List_##Suffix##_index_of (\
        const List_##Suffix* self,\
        const T* target\
) {\
        if (self && self->impl && self->impl->vtbl)\
                return self->impl->vtbl->index_of(self->impl, target);\
        return -1;\
}\

#endif
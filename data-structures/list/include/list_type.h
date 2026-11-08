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
 *
 * 生成的 List_Suffix 独占其 impl 指向的链表，不要通过赋值复制句柄；
 * 使用操作函数时传入句柄地址。new 失败时 impl 为 NULL，destroy 成功后
 * 会将该句柄的 impl 置为 NULL。元素所有权规则与 ListVtbl 一致。
 *
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
static const ListTypeOps ListTypeOps_##Suffix = {\
        .equals = (EqualsFn),\
        .copy   = (CopyFn),\
        .drop   = (DropFn)\
};\
\
static inline List_##Suffix List_##Suffix##_new(\
        ListKind kind\
) {\
        List_##Suffix result = {\
                .impl = create_list(\
                        kind,\
                        sizeof(T),\
                        &ListTypeOps_##Suffix\
                )\
        };\
        return result;\
}\
\
static inline int List_##Suffix##_push_front(\
        List_##Suffix* self,\
        const T* elem\
) {\
        return (\
                self &&\
                self->impl &&\
                self->impl->vtbl &&\
                self->impl->vtbl->push_front &&\
                self->impl->vtbl->push_front(self->impl, elem)\
        );\
}\
\
static inline int List_##Suffix##_push_back(\
        List_##Suffix* self,\
        const T* elem\
) {\
        return (\
                self &&\
                self->impl &&\
                self->impl->vtbl &&\
                self->impl->vtbl->push_back &&\
                self->impl->vtbl->push_back(self->impl, elem)\
        );\
}\
\
static inline T* List_##Suffix##_pop_front(\
        List_##Suffix* self\
) {\
        if (\
                self &&\
                self->impl &&\
                self->impl->vtbl &&\
                self->impl->vtbl->pop_front\
        ) return (T*)self->impl->vtbl->pop_front(self->impl);\
\
        return NULL;\
}\
\
static inline T* List_##Suffix##_pop_back(\
        List_##Suffix* self\
) {\
        if (\
                self &&\
                self->impl &&\
                self->impl->vtbl &&\
                self->impl->vtbl->pop_back\
        ) return (T*)self->impl->vtbl->pop_back(self->impl);\
\
        return NULL;\
}\
\
static inline int List_##Suffix##_remove(\
        List_##Suffix* self,\
        const size_t index\
) {\
        return (\
                self &&\
                self->impl &&\
                self->impl->vtbl &&\
                self->impl->vtbl->remove &&\
                self->impl->vtbl->remove(self->impl, index)\
        );\
}\
\
static inline int List_##Suffix##_destroy(\
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
static inline const T* List_##Suffix##_get(\
        const List_##Suffix* self,\
        const size_t index\
) {\
        if (\
                self &&\
                self->impl &&\
                self->impl->vtbl &&\
                self->impl->vtbl->get\
        ) return (const T*)self->impl->vtbl->get(self->impl, index);\
\
        return NULL;\
}\
\
static inline int List_##Suffix##_index_of(\
        const List_##Suffix* self,\
        size_t* index,\
        const T* target\
) {\
        if (\
                self &&\
                self->impl &&\
                self->impl->vtbl &&\
                self->impl->vtbl->index_of\
        ) return self->impl->vtbl->index_of(self->impl, index, target);\
        return 0;\
}\

#endif

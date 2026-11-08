# 通用链表接口

本目录通过函数表提供统一的链表操作接口，并使用 `LIST_TYPE` 为指定元素类型生成包装函数。目前实现了非循环单链表，其余链表类型预留了创建入口。

## 文件结构

| 文件 | 作用 |
| --- | --- |
| [include/list.h](include/list.h) | 通用链表、元素回调和操作表定义 |
| [include/list_type.h](include/list_type.h) | 生成带元素类型的包装接口 |
| [include/slist.h](include/slist.h)、[src/slist.c](src/slist.c) | 单链表的节点、创建与操作 |
| [include/container_of.h](include/container_of.h) | 由成员指针获取外层结构体指针 |
| [src/list.c](src/list.c) | 校验创建参数并选择链表实现 |

## 设计与实现进度

`List` 保存元素大小、元素数量、链表操作表 `ListVtbl` 和元素类型回调 `ListTypeOps`。单链表在 `SList` 中嵌入 `List`，额外保存头尾指针；操作通过公共接口分派到 `slist` 实现。

- `SINGLY`：支持创建、首尾插入、首尾弹出、按索引删除和读取、查找及销毁。
- `DOUBLY`、`CIRCULAR`、`CIRCULAR_DOUBLY`：暂未实现，创建时返回 `NULL`。
- 首尾插入和表头弹出为 `O(1)`；表尾弹出需要寻找前驱节点，为 `O(n)`。按索引访问、删除和查找为 `O(n)`。

## 元素回调与所有权

创建链表时，元素大小必须大于零，`equals` 和 `copy` 必须提供，`drop` 可以为 `NULL`。

- `equals(a, b)`：元素相等时返回非零。
- `copy(dst, src)`：向已分配的 `dst` 复制元素，成功返回非零；失败返回零，并自行回收复制过程中分配的内部资源。包含指针等资源的类型应按需要实现深拷贝。
- `drop(value)`：清理元素内部资源，不释放 `value` 本身。

`push_front` 和 `push_back` 插入元素的副本，原元素仍由调用方管理。`get` 返回链表内部元素的只读借用指针，调用方不能释放它；元素被删除或链表被销毁后指针失效。若该元素被 `pop`，数据的所有权转移给 `pop` 的调用方。

`pop_front` 和 `pop_back` 移除节点并返回元素数据；调用方使用完后，先调用相应的 `drop`（如需要），再 `free` 返回指针。`remove` 和 `destroy` 则由链表清理并释放它仍持有的元素。`index_of` 找到时返回 `1` 并写入索引；未找到或参数无效时返回 `0`，不修改输出索引。

包装句柄独占链表，不应通过结构体赋值复制；通过地址传递给包装函数。`destroy` 成功后会将该句柄的 `impl` 置为 `NULL`。

## 使用示例

```c
#include <stdio.h>
#include <stdlib.h>

#include "list_type.h"

static int IntEquals(const void* a, const void* b) {
        return *(const int*)a == *(const int*)b;
}

static int IntCopy(void* dst, const void* src) {
        *(int*)dst = *(const int*)src;
        return 1;
}

LIST_TYPE(int, Int, IntEquals, IntCopy, NULL)

int main(void) {
        List_Int list = List_Int_new(SINGLY);
        if (!list.impl) return 1;

        int value = 42;
        if (!List_Int_push_back(&list, &value)) {
                List_Int_destroy(&list);
                return 1;
        }

        size_t index;
        if (List_Int_index_of(&list, &index, &value)) {
                const int* found = List_Int_get(&list, index);
                if (found) printf("index=%zu, value=%d\n", index, *found);
        }

        int* popped = List_Int_pop_front(&list);
        free(popped); /* int 不需要 drop */
        List_Int_destroy(&list);
        return 0;
}
```

将示例文件放置于 `data-structures/list/example.c`。在仓库根目录编译：

```bash
cc -std=gnu11 -Wall -Wextra -I data-structures/list/include \
  data-structures/list/example.c data-structures/list/src/list.c data-structures/list/src/slist.c \
  -o list-example
./list-example
```

目前 `container_of` 使用 GNU C 扩展，因此示例采用 `gnu11` 模式。

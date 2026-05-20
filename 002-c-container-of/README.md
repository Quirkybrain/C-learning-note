# 从首成员基类到 container_of：C 语言里的封装抽象再进一步

001-c-polymorphism-with-vtable 的示例已经展示了 C 语言如何通过结构体和函数指针表模拟封装、抽象与多态。这一次对于 `cat.c` 做了一个很关键的修改：`Animal base` 不再是 `struct Cat` 的第一个成员，而是放在了 `lives` 后面。

这个变化看起来很小，但它让示例从“刚好能工作”推进到了更接近真实工程里的写法。因为对象转换不再依赖“基类字段必须放在结构体第一个位置”这个隐含约定，而是通过宏根据成员地址反推出外层对象地址。

## 构建与运行

这个示例同样使用 `include/` 和 `src/` 目录组织。由于 `container_of()` 版本依赖 GNU C 扩展里的 `typeof` 和 statement expression，Makefile 显式使用了 `-std=gnu11`。

```bash
make
make run
```

## 改了什么

001-c-polymorphism-with-vtable 里，`Cat` 的结构是这样：

```c
struct Cat {
        Animal base;
};
```

我们在 `base` 后添加新的字段 `lives`：

```c
struct Cat {
        Animal base;
        int lives;
};
```

因为 `base` 是第一个成员，所以 `Cat*` 和 `&cat->base` 在地址数值上相同。于是，在 `catSpeak()` 里可以直接写：

```c
static void catSpeak(Animal* self) {
        Cat* cat = (Cat*) self;
        printf("miaow~ I am %s, a cat, with %d lives.\n", self->name, cat->lives);
}
```

编译运行得到：

```text
I am Tom. (Init a cat)
I am Max. (Init a dog)
miaow~ Tom drink water.
woof~ Max drink water.
miaow~ I am Tom, a cat, with 9 lives.
woof~ I am Max, a dog.
```

这依赖一个非常强的前提：`Animal base` 必须永远是 `struct Cat` 的第一个字段。一旦把字段 `lives` 添加在 `base` 前面：

```c
struct Cat {
        int lives;
        Animal base;
};
```

`Cat*` 指向整个对象的起始位置，而 `Animal* self` 指向对象内部的 `base` 成员。两者地址已经不相同了。此时如果继续把 `Animal*` 强制转换成 `Cat*`，得到的就不是正确的 `Cat` 起始地址，后续访问 `cat->lives` 是未定义行为，可能读到错误值，也可能出现其他不可预测结果：

```text
I am Tom. (Init a cat)
I am Max. (Init a dog)
miaow~ Tom drink water.
woof~ Max drink water.
miaow~ I am Tom, a cat, with 7171924 lives.
woof~ I am Max, a dog.
```

解决这个问题的核心是：

```c
/**
 * 使用 container_of() 根据成员指针恢复外层对象
 */
Cat* cat = container_of(self, Cat, base);
```

`self` 仍然是抽象接口传进来的 `Animal*`，但 `container_of()` 会根据 `base` 成员在 `Cat` 结构体中的偏移量，把这个成员指针反推回真正的 `Cat*`。

## Macro 是怎么定位的

宏定义如下：

```c
/**
 * 当前 container_of 使用了 GNU C 扩展。
 * typeof 和 ({ ... }) 不是 ISO C 标准语法，
 * GCC/Clang 常见可用，
 * 但严格 C11 或 MSVC 不一定支持
 */
#define my_offsetof(type, member) ((size_t)&(((type*)0)->member))

#define container_of(ptr, type, member) ({                  \
    const typeof( ((type*)0)->member )* __mptr = (ptr);    \
    (type*)( (char*)__mptr - my_offsetof(type, member) ); \
})
```

先看 `my_offsetof(type, member)`：

```c
/**
 * 手写 my_offsetof 作为学习拆解
 * 生产代码应优先使用 <stddef.h> 的 offsetof
 */
((size_t)&(((type*)0)->member))
```

它的思路是：假设有一个 `type*` 指针，它的地址是 `0`，那么 `((type*)0)->member` 这个成员的地址，数值上就等于这个成员相对于结构体起点的偏移量。

举个例子：

```c
struct Cat {
        int lives;
        Animal base;
};
```

如果编译器决定 `base` 位于 `Cat` 对象起点之后的第 8 个字节，那么：

```c
my_offsetof(Cat, base)
```

得到的就是 `8`。这里的 `8` 只是假设，真实偏移应该交给编译器计算，因为结构体对齐和填充都由编译器负责。

再看 `container_of(ptr, type, member)`：

```c
const typeof( ((type*)0)->member )* __mptr = (ptr);
```

这一行先把传入的成员指针保存到 `__mptr`。`typeof(((type *)0)->member)` 会取出 `member` 的类型。在当前例子里，`member` 是 `base`，所以它的类型是 `Animal`，`__mptr` 的类型就是 `const Animal*`。

这样写有两个好处：

- `ptr` 只会被求值一次，避免宏参数带副作用时重复执行。
- 编译器能帮我们做一层类型检查。如果你传进来的指针类型和 `member` 对不上，编译阶段更容易暴露问题。

最后一行是真正的地址反推：

```c
(type*)( (char*)__mptr - my_offsetof(type, member) )
```

这里一定要先转成 `char*`，因为 C 语言里的指针加减是按指向类型的大小移动的。`char` 的大小就是 1 字节，所以 `char*` 做减法时，减去的就是准确的字节数。

完整过程可以理解为：

```text
Cat 对象起点
    |
    |  偏移 offsetof(Cat, base)
    v
Animal base 成员地址，也就是 self
```

所以反推时只要反过来：

```text
self - offsetof(Cat, base) = Cat 对象起点
```

这就是 `container_of(self, Cat, base)` 的本质。

## 这种封装抽象的好处

第一，具体类型的内存布局不再被抽象层绑死。

001-c-polymorphism-with-vtable 里，为了让 `(Cat*)self` 成立，`Animal base` 必须是第一个字段。这其实把 `Cat` 的内部布局暴露给了多态机制。本节里，只要 `catAsAnimal()` 返回的是 `&cat->base`，`catSpeak()` 再用 `container_of()` 反推回来，`base` 放在第一个、第二个，甚至更后面都可以。

第二，具体类型可以更自然地拥有自己的私有状态。

现在 `Cat` 增加了：

```c
int lives;
```

而且 `catSpeak()` 可以在拿到抽象的 `Animal*` 后重新找回 `Cat*`：

```c
Cat* cat = container_of(self, Cat, base);
printf("miaow~ I am %s, a cat, with %d lives.\n", self->name, cat->lives);
```

调用端依然只知道 `Animal*`，但具体实现内部可以访问 `Cat` 自己的数据。这正是封装和多态一起工作时很重要的一点：公共接口保持抽象，具体实现保留自己的状态和行为。

第三，公开接口更稳定。

`cat.h` 只暴露了不透明类型：

```c
typedef struct Cat Cat;
```

外部代码不知道 `struct Cat` 里到底有几个字段，也不知道 `base` 在哪里。以后给 `Cat` 增加 `lives`、`color`、`age`，或者调整字段顺序，调用端都不需要跟着改。这就是把变化限制在 `.c` 文件内部的价值。

第四，代码更接近一些底层工程里的真实写法。

Linux kernel 里大量使用类似 `container_of` 的技巧。典型场景是：某个通用模块只拿到结构体里的一个公共成员指针，比如链表节点、引用计数对象、工作队列节点，然后再反推出真正拥有这个成员的外层对象。这一章中 `cat.c` 添加了这个思想。


## 完整 cat.c 展示

```c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include "cat.h"


/**
 * @brief 教学用的偏移量宏拆解版本。
 *
 * 生产代码优先使用标准库里的 offsetof(type, member)。这里手写一个
 * 版本，是为了把“成员相对结构体起点的字节偏移”这个概念显式展开。
 */
#define my_offsetof(type, member) ((size_t)&(((type*)0)->member))

/**
 * @brief 通过成员指针反推出外层对象指针。
 *
 * ptr    : 指向结构体某个成员的指针。
 * type   : 外层结构体类型。
 * member : ptr 对应的那个成员名。
 *
 * 这个版本使用了 GNU C 的 typeof 和 statement expression 扩展，
 * 所以 Makefile 里显式使用了 gnu11 模式。
 */
#define container_of(ptr, type, member) ({                  \
    const typeof( ((type*)0)->member )* __mptr = (ptr);    \
    (type*)( (char*)__mptr - my_offsetof(type, member) ); \
})


/**
 * @brief 具体的 Cat 对象。
 *
 * 这里故意把 Cat 的私有字段 lives 放到 Animal base 前面，目的是
 * 演示：一旦基类不再位于结构体起始处，就不能把 Animal* 直接强转成
 * Cat*，而必须通过 container_of() 根据成员偏移量反推出完整对象。
 */
struct Cat {
        /** @brief Cat 自己的私有状态，用来证明对象里可以有额外字段。 */
        int lives;

        /** @brief 公开给抽象层使用的公共基类部分。 */
        Animal base;
};


/**
 * @brief 初始化一个新分配出来的 Cat 对象。
 *
 * @param self 要初始化的 Cat 对象。
 * @param name 要复制到 Animal 基类中的名称。
 */
static void catInit(Cat* self, const char* name) {
        strncpy(self->base.name, name, MAX_NAME_LEN - 1);
        self->base.name[MAX_NAME_LEN-1] = 0;
        self->lives = 9;
        printf("I am %s. (Init a cat)\n", self->base.name);
}


/**
 * @brief AnimalVtbl::speak 的 Cat 实现。
 *
 * 抽象层回调传进来的只有 Animal*。由于 base 不是第一个成员，
 * self 不等于 Cat 对象的起始地址，所以这里必须先用 container_of()
 * 找回真正的 Cat*，才能安全访问 lives。
 *
 * @param self 属于 Cat 对象的 Animal 基类指针。
 */
static void catSpeak(Animal* self) {
        /* 直接强转会得到错误地址；需要根据 base 的偏移量反推完整对象。 */
        Cat* cat = container_of(self, Cat, base);
        printf("miaow~ I am %s, a cat, with %d lives.\n", self->name, cat->lives);
}


/**
 * @brief AnimalVtbl::drink 的 Cat 实现。
 *
 * @param self 属于 Cat 对象的 Animal 基类指针。
 */
static void catDrink(Animal* self) {
        printf("miaow~ %s drink water.\n", self->name);
}

/**
 * @brief 将抽象层定义的 speak/drink 操作绑定到 Cat 的具体实现。
 */
static const AnimalVtbl catVtbl = {
        .speak = catSpeak,
        .drink = catDrink
};

/**
 * @brief 分配并初始化一个 Cat 对象。
 *
 * @param name 要复制到内嵌 Animal 基类对象中的名称。
 * @return 指向新 Cat 对象的指针；如果分配失败则返回 NULL。
 */
Cat* newCat(const char* name) {
        Cat* cat = (Cat*) malloc (sizeof(Cat));
        if (cat == NULL) return NULL;

        /* 多态分发依赖这张表，因此对象创建后第一时间就要绑定。 */
        cat->base.vtblptr = &catVtbl;
        catInit(cat, name);
        
        return cat;
}

/**
 * @brief 释放由 newCat() 创建的 Cat 对象。
 *
 * @param cat 要释放的 Cat 对象。
 */
void deleteCat(Cat* cat) {
        free(cat);
}

/**
 * @brief 将 Cat 视为其内嵌的 Animal 基类对象。
 *
 * 这里返回的是对象内部 base 成员的地址，而不是 Cat 对象本身的起始
 * 地址。调用端只拿到抽象视图，具体实现内部再通过 container_of()
 * 恢复成完整对象。
 *
 * @param cat 要转换的 Cat 对象。
 * @return 指向内嵌 Animal 基类对象的指针。
 */
Animal* catAsAnimal(Cat* cat) {
        return &(cat->base);
}
```

## 小结

这章提到的内容也是 C 语言很有意思的地方。它没有内建的类系统，但它足够贴近内存模型。只要我们理解地址、偏移量、函数指针和结构体布局，就可以在很薄的一层机制上搭出可读、可维护的抽象。

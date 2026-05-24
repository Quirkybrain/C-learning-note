#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include "cat.h"


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
        (type*)( (char*)__mptr - offsetof(type, member) ); \
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
 * @brief AnimalVtbl::release 的 Cat 实现。
 *
 * Cat 当前没有像 Dog 那样额外申请堆资源，因此第二阶段仍然只是
 * 释放完整对象本体。
 *
 * @param self 属于 Cat 对象的 Animal 基类指针。
 */
static void releaseCat(Animal* self) {
        Cat* cat = container_of(self, Cat, base);
        printf("I am %s. (destroy a cat)\n", self->name);
        free(cat);
}

/**
 * @brief AnimalVtbl::cleanUp 的 Cat 实现。
 *
 * 这个例子里 Cat 没有需要单独清理的私有资源，但仍然保留 cleanUp
 * 槽位，用来体现 004 统一的“两阶段销毁协议”。
 *
 * @param self 属于 Cat 对象的 Animal 基类指针。
 */
static void cleanUpCat(Animal* self) {
        (void)self;
                printf("(destroy Cat's member if have)\n");
        return;
}

/**
 * @brief 将抽象层定义的 speak/drink/release/cleanUp 操作绑定到 Cat 的具体实现。
 */
static const AnimalVtbl catVtbl = {
        .speak = catSpeak,
        .drink = catDrink,
        .release = releaseCat,
        .cleanUp = cleanUpCat
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include "dog.h"


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
 * @brief 具体的 Dog 对象。
 *
 * Dog 仍然把 Animal base 放在第一个成员位置，和前一个示例保持一致。
 * 这样可以和 Cat 的“base 不在首位”形成对比，帮助观察两种布局的差异。
 */
struct Dog {
        /** @brief 公开给抽象层使用的公共基类部分。 */
        Animal base;
};


/**
 * @brief 初始化一个新分配出来的 Dog 对象。
 *
 * @param self 要初始化的 Dog 对象。
 * @param name 要复制到 Animal 基类中的名称。
 */
static void dogInit(Dog* self, const char* name) {
        strncpy(self->base.name, name, MAX_NAME_LEN - 1);
        self->base.name[MAX_NAME_LEN-1] = 0;
        printf("I am %s. (Init a dog)\n", self->base.name);
}

/**
 * @brief AnimalVtbl::speak 的 Dog 实现。
 *
 * Dog 的行为实现只依赖公共基类里的名称，因此直接使用 self->name
 * 就足够了。
 *
 * @param self 属于 Dog 对象的 Animal 基类指针。
 */
static void dogSpeak(Animal* self) {
        printf("woof~ I am %s, a dog.\n", self->name);
}

/**
 * @brief AnimalVtbl::drink 的 Dog 实现。
 *
 * @param self 属于 Dog 对象的 Animal 基类指针。
 */
static void dogDrink(Animal* self) {
        printf("woof~ %s drink water.\n", self->name);
}

/**
 * @brief AnimalVtbl::destroy 的 Dog 实现。
 *
 * 虽然 Dog 当前把 base 放在第一个成员位置，仍然统一使用 container_of()
 * 恢复完整对象。这样即使以后 Dog 增加私有字段并调整布局，析构路径也
 * 不需要依赖“base 必须在首位”的隐含约定。
 *
 * @param self 指向 Dog 对象内嵌 Animal 基类指针的地址。
 */
static void destroyDog(Animal** self) {
        Dog* dog = container_of(*self, Dog, base);
        printf("I am %s. (destroy a dog)\n", (*self)->name);
        free(dog);
        *self = NULL;
}


/**
 * @brief 将抽象层定义的 speak/drink/destroy 操作绑定到 Dog 的具体实现。
 */
static const AnimalVtbl dogVtbl = {
        .speak = dogSpeak,
        .drink = dogDrink,
        .destroy = destroyDog
};

/**
 * @brief 分配并初始化一个 Dog 对象。
 *
 * @param name 要复制到内嵌 Animal 基类对象中的名称。
 * @return 指向新 Dog 对象的指针；如果分配失败则返回 NULL。
 */
Dog* newDog(const char* name) {
        Dog* dog = (Dog*) malloc (sizeof(Dog));
        if (dog == NULL) return NULL;

        /* 新对象创建完成后，先让抽象层知道它该走哪张行为表。 */
        dog->base.vtblptr = &dogVtbl;
        dogInit(dog, name);
        
        return dog;
}

/**
 * @brief 将 Dog 视为其内嵌的 Animal 基类对象。
 *
 * 因为 base 位于结构体起始处，这里返回的地址与 dog 本身起始地址一致；
 * 但对调用方来说，暴露出去的仍然只是更抽象的 Animal*。
 *
 * @param dog 要转换的 Dog 对象。
 * @return 指向内嵌 Animal 基类对象的指针。
 */
Animal* dogAsAnimal(Dog* dog) {
        return &(dog->base);
}

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
 * 这次把 Dog 的私有字段 foodName 放到 Animal base 前面，目的是
 * 演示：当具体类型除了对象本体外，还额外拥有自己申请的资源时，
 * 销毁过程就需要先清理这些资源，再释放整个对象。
 */
struct Dog {
        /** @brief Dog 私有的堆内存字符串，用来演示析构链的第一阶段。 */
        char* foodName;
        /** @brief 公开给抽象层使用的公共基类部分。 */
        Animal base;
};


/**
 * @brief 初始化一个新分配出来的 Dog 对象。
 *
 * @param self 要初始化的 Dog 对象。
 * @param name 要复制到 Animal 基类中的名称。
 * @param food 要复制到 Dog 私有 foodName 缓冲区中的食物名称。
 */
static void dogInit(Dog* self, const char* name, const char* food) {
        strncpy(self->base.name, name, MAX_NAME_LEN - 1);
        self->base.name[MAX_NAME_LEN-1] = 0;

        /* Dog 现在额外拥有一段堆内存，因此初始化阶段也要显式分配并拷贝。 */
        self->foodName = (char*) calloc (MAX_DOG_FOOD_NAME, sizeof(char));
        strncpy(self->foodName, food, MAX_DOG_FOOD_NAME-1);
        self->foodName[MAX_DOG_FOOD_NAME-1] = 0;

        printf("I am %s. (Init a dog)\n", self->base.name);
}

/**
 * @brief AnimalVtbl::speak 的 Dog 实现。
 *
 * 004 里 Dog 的行为不再只依赖基类字段；为了访问私有 foodName，
 * 这里也需要先通过 container_of() 找回完整对象。
 *
 * @param self 属于 Dog 对象的 Animal 基类指针。
 */
static void dogSpeak(Animal* self) {
        Dog* dog = container_of(self, Dog, base);
        printf("woof~ I am %s, a dog, like to eat %s.\n", self->name, dog->foodName);
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
 * @brief AnimalVtbl::release 的 Dog 实现。
 *
 * 这个阶段只负责释放完整的 Dog 对象本体；Dog 私有资源的清理工作
 * 已经在 cleanUpDog() 里先一步完成。
 *
 * @param self 属于 Dog 对象的 Animal 基类指针。
 */
static void releaseDog(Animal* self) {
        Dog* dog = container_of(self, Dog, base);
        printf("I am %s. (destroy a dog)\n", self->name);
        free(dog);
}

/**
 * @brief AnimalVtbl::cleanUp 的 Dog 实现。
 *
 * Dog 新增了自己申请的 foodName 堆内存，所以在释放对象本体之前，
 * 要先把这部分私有资源清理掉。
 *
 * @param self 属于 Dog 对象的 Animal 基类指针。
 */
static void cleanUpDog(Animal* self) {
        Dog* dog = container_of(self, Dog, base);
        printf("(destroy Dog's foodName member: %s.)\n", dog->foodName);
        free(dog->foodName);
}

/**
 * @brief 将抽象层定义的 speak/drink/release/cleanUp 操作绑定到 Dog 的具体实现。
 */
static const AnimalVtbl dogVtbl = {
        .speak = dogSpeak,
        .drink = dogDrink,
        .release = releaseDog,
        .cleanUp = cleanUpDog
};

/**
 * @brief 分配并初始化一个 Dog 对象。
 *
 * @param name 要复制到内嵌 Animal 基类对象中的名称。
 * @return 指向新 Dog 对象的指针；如果分配失败则返回 NULL。
 */
Dog* newDog(const char* name, const char* foodName) {
        Dog* dog = (Dog*) malloc (sizeof(Dog));
        if (dog == NULL) return NULL;

        /* 新对象创建完成后，先让抽象层知道它该走哪张行为表。 */
        dog->base.vtblptr = &dogVtbl;
        dogInit(dog, name, foodName);
        
        return dog;
}

/**
 * @brief 将 Dog 视为其内嵌的 Animal 基类对象。
 *
 * 这里返回的是对象内部 base 成员的地址，而不是 Dog 对象本身的起始
 * 地址。调用端只拿到抽象视图；后续无论是行为调用，还是 cleanUp/
 * release 两阶段销毁，都需要在具体实现里再通过 container_of()
 * 恢复成完整对象。
 *
 * @param dog 要转换的 Dog 对象。
 * @return 指向内嵌 Animal 基类对象的指针。
 */
Animal* dogAsAnimal(Dog* dog) {
        return &(dog->base);
}

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cat.h"


/**
 * @brief 具体的 Cat 对象。
 *
 * 这个示例里把 Animal base 放在第一个成员位置，目的是让
 * Cat 对象和其内嵌的 Animal 子对象拥有相同的起始地址。
 * 这样 Cat* 可以很自然地向上转成 Animal* 参与多态分发。
 */
struct Cat {
        /** @brief 公开给抽象层使用的公共基类部分。 */
        Animal base;
};


/**
 * @brief 初始化一个新分配出来的 Cat 对象。
 *
 * 这里会把名字写入公共基类区域。由于这是教学示例，我们只保留
 * 最小初始化逻辑，不再额外拆出更复杂的私有状态。
 *
 * @param self 要初始化的 Cat 对象。
 * @param name 要复制到 Animal 基类中的名称。
 */
static void catInit(Cat* self, const char* name) {
        strncpy(self->base.name, name, MAX_NAME_LEN - 1);
        self->base.name[MAX_NAME_LEN-1] = 0;
        printf("I am %s. (Init a cat)\n", self->base.name);
}

/**
 * @brief AnimalVtbl::speak 的 Cat 实现。
 *
 * 这个回调只访问 Animal 基类里已有的公共字段，因此不需要再把
 * self 向下转换回 Cat*。
 *
 * @param self 属于 Cat 对象的 Animal 基类指针。
 */
static void catSpeak(Animal* self) {
        printf("miaow~ I am %s, a cat.\n", self->name);
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
 *
 * 每个 Cat 对象都会把自己的 base.vtblptr 指向这张静态表。
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

        /* 先绑定行为表，再初始化公共数据，确保对象一创建就具备完整身份。 */
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
 * 因为 base 是第一个成员，所以这里返回的地址与 cat 本身的起始
 * 地址相同；不过对外暴露的类型仍然是更抽象的 Animal*。
 *
 * @param cat 要转换的 Cat 对象。
 * @return 指向内嵌 Animal 基类对象的指针。
 */
Animal* catAsAnimal(Cat* cat) {
        return &(cat->base);
}

#include "animal.h"

/**
 * @brief 使用对象当前绑定的函数表分发 drink 行为。
 *
 * 这层函数本身不包含“猫怎么喝水、狗怎么喝水”的知识，只负责
 * 读取对象里的 vtblptr 并把调用转交给对应实现。
 *
 * @param self 指向具体对象内嵌 Animal 基类的指针。
 */
void animalDrink(Animal* self) {
        self->vtblptr->drink(self);
}

/**
 * @brief 使用对象当前绑定的函数表分发 speak 行为。
 *
 * @param self 指向具体对象内嵌 Animal 基类的指针。
 */
void animalSpeak(Animal* self) {
        self->vtblptr->speak(self);
}

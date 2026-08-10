#include "animal.h"

/**
 * @brief 通过函数表把 drink 调用分发给具体类型。
 *
 * 这个函数是抽象层对外暴露的统一入口。调用方只需要持有
 * Animal*，无需知道它实际来自 Cat 还是 Dog。
 *
 * @param self 指向具体对象内嵌 Animal 基类的指针。
 */
void animalDrink(Animal* self) {
        self->vtblptr->drink(self);
}

/**
 * @brief 通过函数表把 speak 调用分发给具体类型。
 *
 * 这里的 self 会原样传递给具体实现。具体类型如果需要访问
 * 自己的私有字段，可以在实现内部再把这个基类指针恢复成
 * 真正的外层对象指针。
 *
 * @param self 指向具体对象内嵌 Animal 基类的指针。
 */
void animalSpeak(Animal* self) {
        self->vtblptr->speak(self);
}

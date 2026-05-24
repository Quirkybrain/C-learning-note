#include <stddef.h>
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

/**
 * @brief 按两阶段协议销毁一个抽象动物对象。
 *
 * 调用方仍然只需要持有 Animal**；至于具体类型有哪些附加资源、
 * 完整对象地址又该如何释放，都交给虚表中的 cleanUp/release 分别处理。
 *
 * @param self 指向 Animal* 变量的指针；销毁成功后 *self 会被置为 NULL。
 */
void destroyAnimal(Animal** self) {
        (*self)->vtblptr->cleanUp(*self);
        (*self)->vtblptr->release(*self);
        *self = NULL;
}

#include <stddef.h>
#include <assert.h>
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
 * @brief 为已有对象取得一个新的拥有引用。
 *
 * 这个简化示例使用普通整数记录引用数量，只演示单线程环境下的生命周期
 * 规则。每次成功调用都需要在不再使用时配对调用一次 animalPut()。
 *
 * @param self 已经由调用者持有有效引用的 Animal 基类指针。
 * @return 与 self 指向同一对象的新拥有引用。
 */
Animal* animalGet(Animal* self) {
        /* 只能复制仍然存活的引用，不能从 0 重新增加引用计数。 */
        assert(self->refCount > 0);
        ++self->refCount;
        
        return self;
}

/**
 * @brief 释放调用者持有的一次引用。
 *
 * 先把调用方变量清空，再减少对象的引用计数。这样即使对象仍由其他调用者
 * 持有，当前调用者也不会继续误用自己已经释放的引用。计数归零时才真正
 * 清理具体类型的成员资源并释放完整对象。
 *
 * @param self 指向调用方 Animal* 变量的指针。
 */
void animalPut(Animal** self) {
        /* 暂存对象地址，因为下一步会先清空调用方持有的指针。 */
        Animal* object = *self;
        *self = NULL;

        /* 一次 put 只能释放一个当前确实存在的拥有引用。 */
        assert(object->refCount > 0);
        --object->refCount;

        if (object->refCount == 0) {
                /* 最后一个 owner 离开后，先清理成员资源，再释放对象本体。 */
                (object)->vtblptr->cleanUp(object);
                (object)->vtblptr->release(object);
        }
}

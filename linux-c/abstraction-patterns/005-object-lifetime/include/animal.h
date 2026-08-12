/**
 * @file animal.h
 * @author quirkybrain
 * @brief 用结构体和函数指针表模拟多态时使用的抽象基类接口。
 * @version 0.1
 * @date 2026-05-21
 */
#ifndef ANIMAL_H_
#define ANIMAL_H_


/** @brief 动物名称可用的最大字节数，包含结尾的 '\0'。 */
#define MAX_NAME_LEN 24


/** @brief 抽象动物基类的前向声明。 */
typedef struct Animal Animal;

/** @brief 描述“动物对象支持哪些操作”的函数表类型。 */
typedef struct AnimalVtbl AnimalVtbl;

/**
 * @brief 具体动物类型需要提供的行为表。
 *
 * 抽象层只和 Animal* 以及这张表打交道，因此这里承担了“手写虚表”
 * 的角色。
 */
struct AnimalVtbl {
        /** @brief 让动物发出叫声。self 指向具体对象内嵌的 Animal 基类。 */
        void (*speak)(Animal* self);

        /** @brief 让动物喝水。self 指向具体对象内嵌的 Animal 基类。 */
        void (*drink)(Animal* self);

        /**
         * @brief 清理具体对象额外持有的资源，但不释放对象本体。
         *
         * 004 把销毁动作拆成两个阶段：先 cleanUp 私有资源，再 release
         * 完整对象本体。这样 Dog 之类带堆成员的类型就能先处理成员资源。
         */
        void (*cleanUp)(Animal* self);

        /**
         * @brief 释放完整的具体对象本体。
         *
         * 这个阶段只负责回收最外层对象地址本身；具体类型的附加资源应该
         * 已经在 cleanUp 阶段处理完毕。
         */
        void (*release)(Animal* self);
};

/**
 * @brief 嵌入到每个具体动物类型中的公共基类对象。
 *
 * 具体类型会把这个结构体当作自己的一个成员嵌入进去。调用方拿到的
 * Animal* 实际上通常都指向这个成员，而不是直接指向完整对象。
 */
struct Animal {
        /** @brief 动物的显示名称。 */
        char name[MAX_NAME_LEN];

        /** @brief 指向具体类型函数表的指针，用于运行时行为分发。 */
        const AnimalVtbl* vtblptr;

        /** @brief 当前拥有这个对象的有效引用数量。 */
        unsigned int refCount;
};

/**
 * @brief 通过对象的函数表分发 speak 操作。
 *
 * @param self Animal 基类指针，通常来自某个具体对象内部的 base 成员。
 */
void animalSpeak(Animal* self);

/**
 * @brief 通过对象的函数表分发 drink 操作。
 *
 * @param self Animal 基类指针，通常来自某个具体对象内部的 base 成员。
 */
void animalDrink(Animal* self);

/**
 * @brief 为已有的 Animal 对象取得一个新的拥有引用。
 *
 * 调用者必须已经持有一个有效引用，不能用这个函数让引用计数已经归零的
 * 对象重新存活。返回的新引用最终必须与一次 animalPut() 配对。
 *
 * @param self 已经由调用者持有有效引用的 Animal 基类指针。
 * @return 与 self 指向同一对象的 Animal 基类指针。
 */
Animal* animalGet(Animal* self);

/**
 * @brief 释放调用者持有的一次引用，并把调用方指针置为 NULL。
 *
 * 引用计数仍大于 0 时对象继续存活；最后一个引用被释放时，抽象层会依次
 * 调用具体类型提供的 cleanUp 和 release 完成最终销毁。
 *
 * @param self 指向调用方 Animal* 变量的指针。
 */
void animalPut(Animal** self);

#endif

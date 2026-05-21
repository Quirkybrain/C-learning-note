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
         * @brief 销毁完整的具体动物对象，并把调用方持有的 Animal* 置空。
         *
         * 这里接收 Animal**，是因为 destroy 实现不仅要释放对象，还要把
         * 外部保存的基类指针改成 NULL，降低释放后继续误用悬空指针的风险。
         */
        void (*destroy)(Animal** self);
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
 * @brief 通过对象的函数表分发 destroy 操作。
 *
 * 003 里释放对象也走抽象接口：调用方只需要持有 Animal* 的地址，
 * 不需要判断它实际来自 Cat 还是 Dog。
 *
 * @param self 指向 Animal* 变量的指针；销毁成功后 *self 会被置为 NULL。
 */
void destroyAnimal(Animal** self);

#endif

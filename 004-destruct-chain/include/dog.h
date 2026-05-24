/**
 * @file dog.h
 * @author quirkybrain
 * @brief 具体 Dog 类型的公开接口。
 * @version 0.1
 * @date 2026-05-21
 */
#ifndef DOG_H_
#define DOG_H_


#include "animal.h"

#define MAX_DOG_FOOD_NAME 12


/** @brief 不透明的具体狗类型。调用方只能通过公开函数操作它。 */
typedef struct Dog Dog;

/**
 * @brief 分配并初始化一个 Dog 对象。
 *
 * 创建出的对象可以通过 dogAsAnimal() 放进 Animal* 抽象接口中；
 * 生命周期结束时由 destroyAnimal() 统一分发到 Dog 的析构实现。
 *
 * @param name 要复制到内嵌 Animal 基类对象中的名称。
 * @param food 要复制到 Dog 私有 foodName 缓冲区中的食物名称。
 * @return 指向新 Dog 对象的指针；如果分配失败则返回 NULL。
 */
Dog* newDog(const char* name, const char* food);

/**
 * @brief 将 Dog 对象视为其内嵌的 Animal 基类对象。
 *
 * @param dog 要转换的 Dog 对象。
 * @return 指向内嵌 Animal 基类对象的指针。返回值仅在 dog 存活期间有效。
 */
Animal* dogAsAnimal(Dog* dog);

#endif

/**
 * @file cat.h
 * @author quirkybrain
 * @brief 具体 Cat 类型的公开接口。
 * @version 0.1
 * @date 2026-05-21
 */
#ifndef CAT_H_
#define CAT_H_


#include "animal.h"


/** @brief 不透明的具体猫类型。调用方只能通过公开函数操作它。 */
typedef struct Cat Cat;

/**
 * @brief 分配并初始化一个 Cat 对象。
 *
 * 创建出的对象可以通过 catAsAnimal() 放进 Animal* 抽象接口中；
 * 生命周期结束时由 destroyAnimal() 统一分发到 Cat 的析构实现。
 *
 * @param name 要复制到内嵌 Animal 基类对象中的名称。
 * @return 指向新 Cat 对象的指针；如果分配失败则返回 NULL。
 */
Cat* newCat(const char* name);

/**
 * @brief 将 Cat 对象视为其内嵌的 Animal 基类对象。
 *
 * @param cat 要转换的 Cat 对象。
 * @return 指向内嵌 Animal 基类对象的指针。返回值仅在 cat 存活期间有效。
 */
Animal* catAsAnimal(Cat* cat);

#endif

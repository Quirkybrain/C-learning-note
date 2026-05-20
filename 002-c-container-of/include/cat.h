/**
 * @file cat.h
 * @author quirkybrain
 * @brief 具体 Cat 类型的公开接口。
 * @version 0.1
 * @date 2026-05-20
 */
#ifndef _CAT_H
#define _CAT_H


#include "animal.h"


/** @brief 不透明的具体猫类型。调用方只能通过公开函数操作它。 */
typedef struct Cat Cat;

/**
 * @brief 分配并初始化一个 Cat 对象。
 *
 * @param name 要复制到内嵌 Animal 基类对象中的名称。
 * @return 指向新 Cat 对象的指针；如果分配失败则返回 NULL。
 */
Cat* newCat(const char* name);

/**
 * @brief 释放由 newCat() 创建的 Cat 对象。
 *
 * @param cat 要释放的 Cat 对象。
 */
void deleteCat(Cat* cat);

/**
 * @brief 将 Cat 对象视为其内嵌的 Animal 基类对象。
 *
 * @param cat 要转换的 Cat 对象。
 * @return 指向内嵌 Animal 基类对象的指针。返回值仅在 cat 存活期间有效。
 */
Animal* catAsAnimal(Cat* cat);

#endif

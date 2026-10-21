/**
 * @file container_of.h
 * @author quirkybrain
 * @brief 提供通过成员指针获取其所属外层结构体指针的通用宏
 * @version 0.1
 * @date 2026-08-14
 * 
 * @copyright Copyright (c) 2026
 * 
 */
#ifndef CONTAINER_OF_H_
#define CONTAINER_OF_H_

#include <stddef.h>

/**
 * @brief 通过成员指针反推出外层对象指针
 *
 * @param ptr 指向结构体某个成员的指针
 * @param type 外层结构体类型
 * @param member ptr 对应的成员名
 * @return 外层对象的指针
 */
#define container_of(ptr, type, member) ({                       \
        const typeof(((type*)0)->member)* __mptr = (ptr);        \
        (type*)((char*)__mptr - offsetof(type, member));         \
})

#endif
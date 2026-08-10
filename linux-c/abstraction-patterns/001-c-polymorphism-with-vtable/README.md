# 用结构体和函数指针实现封装、抽象与多态：C 语言中的函数表

- 作者：Quirkybrain
- GitHub 仓库：[Quirkybrain/C-learning-note](https://github.com/Quirkybrain/C-learning-note)

C 语言本身没有 `class`、`virtual`、`override` 这些语法，也没有内建的面向对象机制。但这并不意味着 C 不能表达“封装”“抽象”和“多态”这些设计思想。

这份示例通过一个简单的 `Animal` / `Cat` / `Dog` 模型展示一种常见写法：

- 用结构体保存对象的公共数据。
- 用函数指针表描述一组抽象行为。
- 让具体类型在自己的实现文件里提供函数表。
- 调用端只通过抽象接口调用行为，由运行时对象内部的函数表决定真正执行哪个函数。

最终效果是：同样调用 `animalDrink()` 和 `animalSpeak()`，传入猫对象时执行猫的行为，传入狗对象时执行狗的行为。

## 构建与运行

当前工程按 `include/` 和 `src/` 目录组织，直接在目录内执行：

```bash
make
make run
```

## 整体结构

这个示例由以下文件组成：

```text
.
├── include
│   ├── animal.h   # 抽象层：Animal 基类与函数表定义
│   ├── cat.h      # Cat 的公开接口
│   └── dog.h      # Dog 的公开接口
├── src
│   ├── animal.c   # 抽象接口的分发实现
│   ├── cat.c      # Cat 的私有结构、函数表和具体行为
│   └── dog.c      # Dog 的私有结构、函数表和具体行为
├── main.c         # 调用端示例
└── Makefile       # 构建脚本
```

其中最重要的抽象关系是：

```text
Cat / Dog 对象
    ↓ 内嵌
Animal base
    ↓ 持有
AnimalVtbl* vtblptr
    ↓ 指向
具体类型自己的函数表 catVtbl / dogVtbl
    ↓ 分发
catSpeak / dogSpeak
catDrink / dogDrink
```

## 抽象层：Animal 与函数表

`animal.h` 是整个设计的核心。它定义了两个关键结构：

- `AnimalVtbl`：函数表，描述“动物应该支持哪些行为”。
- `Animal`：公共基类对象，保存名称和指向函数表的指针。

这里的 `AnimalVtbl` 很像 C++ 对象背后的虚函数表。区别在于，C 不会自动帮我们生成和维护它，一切都要手动写出来。

### animal.h

```c
/**
 * @file animal.h
 * @author quirkybrain
 * @brief 用结构体和函数指针表模拟多态时使用的抽象基类接口。
 * @version 0.1
 * @date 2026-05-20
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
 * 调用端永远只通过 Animal* 和这张表交互，不直接依赖 Cat/Dog
 * 的具体实现函数，因此它在这个示例里承担了“抽象接口”的角色。
 */
struct AnimalVtbl {
        /** @brief 让动物发出叫声。self 指向具体对象内嵌的 Animal 基类。 */
        void (*speak)(Animal* self);

        /** @brief 让动物喝水。self 指向具体对象内嵌的 Animal 基类。 */
        void (*drink)(Animal* self);
};

/**
 * @brief 嵌入到每个具体动物类型中的公共基类对象。
 *
 * Cat 和 Dog 都会把这个结构体作为自己的一个成员嵌入进去。
 * 抽象层只认识 Animal，因此这里存放了跨具体类型共享的最小公共数据：
 * 名称，以及一张指向具体行为实现的函数表。
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

#endif
```

这里的抽象点在于：`animalSpeak()` 和 `animalDrink()` 并不关心传进来的到底是 `Cat` 还是 `Dog`。它们只要求传入一个 `Animal*`，然后通过 `self->vtblptr` 找到真正的行为实现。

### animal.c

```c
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
```

这两个函数就是多态分发的入口：

```c
self->vtblptr->drink(self);
self->vtblptr->speak(self);
```

它们不直接调用 `catDrink()`、`dogDrink()`、`catSpeak()` 或 `dogSpeak()`，而是通过对象内部保存的函数表间接调用。这样，同一个接口就能根据对象的实际类型表现出不同的行为。

## 具体类型：Cat

`Cat` 的公开头文件只暴露“不透明类型”和少量操作函数。调用端知道有一个 `Cat` 类型，但不知道 `struct Cat` 内部是什么样子。

这就是 C 里常见的封装方式：在 `.h` 里只声明类型，在 `.c` 里定义结构体细节。

### cat.h

```c
/**
 * @file cat.h
 * @author quirkybrain
 * @brief 具体 Cat 类型的公开接口。
 * @version 0.1
 * @date 2026-05-20
 */
#ifndef CAT_H_
#define CAT_H_


#include "animal.h"


/** @brief 不透明的具体猫类型。调用方只能通过公开函数操作它。 */
typedef struct Cat Cat;

/**
 * @brief 分配并初始化一个 Cat 对象。
 *
 * @param name 要复制到内嵌 Animal 基类对象中的名称。
 * @return 指向新 Cat 对象的指针；如果内存分配失败则返回 NULL。
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
```

注意这一行：

```c
typedef struct Cat Cat;
```

这里没有给出 `struct Cat` 的字段。因此，对外部调用者来说，`Cat` 是一个不透明类型。调用者不能直接访问猫对象内部的数据，只能通过 `newCat()`、`deleteCat()`、`catAsAnimal()` 这些公开函数操作它。

### cat.c

```c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cat.h"


/**
 * @brief 具体的 Cat 对象。
 *
 * 这个示例里把 Animal base 放在第一个成员位置，目的是让
 * Cat 对象和其内嵌的 Animal 子对象拥有相同的起始地址。
 * 这样 Cat* 可以很自然地向上转成 Animal* 参与多态分发。
 */
struct Cat {
        /** @brief 公开给抽象层使用的公共基类部分。 */
        Animal base;
};


/**
 * @brief 初始化一个新分配出来的 Cat 对象。
 *
 * 这里会把名字写入公共基类区域。由于这是教学示例，我们只保留
 * 最小初始化逻辑，不再额外拆出更复杂的私有状态。
 *
 * @param self 要初始化的 Cat 对象。
 * @param name 要复制到 Animal 基类中的名称。
 */
static void catInit(Cat* self, const char* name) {
        strncpy(self->base.name, name, MAX_NAME_LEN - 1);
        self->base.name[MAX_NAME_LEN-1] = 0;
        printf("I am %s. (Init a cat)\n", self->base.name);
}

/**
 * @brief AnimalVtbl::speak 的 Cat 实现。
 *
 * 这个回调只访问 Animal 基类里已有的公共字段，因此不需要再把
 * self 向下转换回 Cat*。
 *
 * @param self 属于 Cat 对象的 Animal 基类指针。
 */
static void catSpeak(Animal* self) {
        printf("miaow~ I am %s, a cat.\n", self->name);
}

/**
 * @brief AnimalVtbl::drink 的 Cat 实现。
 *
 * @param self 属于 Cat 对象的 Animal 基类指针。
 */
static void catDrink(Animal* self) {
        printf("miaow~ %s drink water.\n", self->name);
}

/**
 * @brief 将抽象层定义的 speak/drink 操作绑定到 Cat 的具体实现。
 *
 * 每个 Cat 对象都会把自己的 base.vtblptr 指向这张静态表。
 */
static const AnimalVtbl catVtbl = {
        .speak = catSpeak,
        .drink = catDrink
};

/**
 * @brief 分配并初始化一个 Cat 对象。
 *
 * @param name 要复制到内嵌 Animal 基类对象中的名称。
 * @return 指向新 Cat 对象的指针；如果分配失败则返回 NULL。
 */
Cat* newCat(const char* name) {
        Cat* cat = (Cat*) malloc (sizeof(Cat));
        if (cat == NULL) return NULL;

        /* 先绑定行为表，再初始化公共数据，确保对象一创建就具备完整身份。 */
        cat->base.vtblptr = &catVtbl;
        catInit(cat, name);
        
        return cat;
}

/**
 * @brief 释放由 newCat() 创建的 Cat 对象。
 *
 * @param cat 要释放的 Cat 对象。
 */
void deleteCat(Cat* cat) {
        free(cat);
}

/**
 * @brief 将 Cat 视为其内嵌的 Animal 基类对象。
 *
 * 因为 base 是第一个成员，所以这里返回的地址与 cat 本身的起始
 * 地址相同；不过对外暴露的类型仍然是更抽象的 Animal*。
 *
 * @param cat 要转换的 Cat 对象。
 * @return 指向内嵌 Animal 基类对象的指针。
 */
Animal* catAsAnimal(Cat* cat) {
        return &(cat->base);
}
```

`Cat` 的关键设计有三处。

第一，`Cat` 内部嵌入了一个 `Animal base`：

```c
struct Cat {
        Animal base;
};
```

这相当于说：猫对象“拥有一个 Animal 基类部分”。公共字段和函数表指针都放在这个 `base` 里。

第二，`catSpeak()` 和 `catDrink()` 是 `static` 函数：

```c
static void catSpeak(Animal* self)
static void catDrink(Animal* self)
```

`static` 让它们只在 `cat.c` 内部可见。外部代码不能直接调用它们，只能通过 `AnimalVtbl` 间接调用。这也是一种封装。

第三，`catVtbl` 把抽象行为绑定到具体实现：

```c
static const AnimalVtbl catVtbl = {
        .speak = catSpeak,
        .drink = catDrink
};
```

当 `newCat()` 创建对象时，会把猫自己的函数表写进 `base.vtblptr`：

```c
cat->base.vtblptr = &catVtbl;
```

从这一刻开始，这个对象被当作 `Animal*` 使用时，调用 `animalSpeak()` 就会走到 `catSpeak()`，调用 `animalDrink()` 就会走到 `catDrink()`。

## 具体类型：Dog

`Dog` 的设计和 `Cat` 几乎一样。区别在于它绑定的是狗自己的行为函数。

这种重复结构正好体现了抽象层的价值：只要具体类型遵守 `AnimalVtbl` 这个“接口约定”，就可以被统一地当作 `Animal` 使用。

### dog.h

```c
/**
 * @file dog.h
 * @author quirkybrain
 * @brief 具体 Dog 类型的公开接口。
 * @version 0.1
 * @date 2026-05-20
 */
#ifndef DOG_H_
#define DOG_H_


#include "animal.h"


/** @brief 不透明的具体狗类型。调用方只能通过公开函数操作它。 */
typedef struct Dog Dog;

/**
 * @brief 分配并初始化一个 Dog 对象。
 *
 * @param name 要复制到内嵌 Animal 基类对象中的名称。
 * @return 指向新 Dog 对象的指针；如果分配失败则返回 NULL。
 */
Dog* newDog(const char* name);

/**
 * @brief 释放由 newDog() 创建的 Dog 对象。
 *
 * @param dog 要释放的 Dog 对象。
 */
void deleteDog(Dog* dog);

/**
 * @brief 将 Dog 对象视为其内嵌的 Animal 基类对象。
 *
 * @param dog 要转换的 Dog 对象。
 * @return 指向内嵌 Animal 基类对象的指针。返回值仅在 dog 存活期间有效。
 */
Animal* dogAsAnimal(Dog* dog);

#endif
```

### dog.c

```c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "dog.h"


/**
 * @brief 具体的 Dog 对象。
 *
 * 和 Cat 一样，Dog 也把 Animal base 放在第一个成员位置。
 * 这保证了 dogAsAnimal() 返回的基类指针能够直接参与统一分发。
 */
struct Dog {
        /** @brief 公开给抽象层使用的公共基类部分。 */
        Animal base;
};


/**
 * @brief 初始化一个新分配出来的 Dog 对象。
 *
 * @param self 要初始化的 Dog 对象。
 * @param name 要复制到 Animal 基类中的名称。
 */
static void dogInit(Dog* self, const char* name) {
        strncpy(self->base.name, name, MAX_NAME_LEN - 1);
        self->base.name[MAX_NAME_LEN-1] = 0;
        printf("I am %s. (Init a dog)\n", self->base.name);
}

/**
 * @brief AnimalVtbl::speak 的 Dog 实现。
 *
 * 这个回调同样只依赖公共基类字段 name，因此不需要访问 Dog
 * 自己的私有状态。
 *
 * @param self 属于 Dog 对象的 Animal 基类指针。
 */
static void dogSpeak(Animal* self) {
        printf("woof~ I am %s, a dog.\n", self->name);
}

/**
 * @brief AnimalVtbl::drink 的 Dog 实现。
 *
 * @param self 属于 Dog 对象的 Animal 基类指针。
 */
static void dogDrink(Animal* self) {
        printf("woof~ %s drink water.\n", self->name);
}

/**
 * @brief 将抽象层定义的 speak/drink 操作绑定到 Dog 的具体实现。
 */
static const AnimalVtbl dogVtbl = {
        .speak = dogSpeak,
        .drink = dogDrink
};

/**
 * @brief 分配并初始化一个 Dog 对象。
 *
 * @param name 要复制到内嵌 Animal 基类对象中的名称。
 * @return 指向新 Dog 对象的指针；如果分配失败则返回 NULL。
 */
Dog* newDog(const char* name) {
        Dog* dog = (Dog*) malloc (sizeof(Dog));
        if (dog == NULL) return NULL;

        /* 新对象先绑定函数表，再写入名称等公共状态。 */
        dog->base.vtblptr = &dogVtbl;
        dogInit(dog, name);
        
        return dog;
}

/**
 * @brief 释放由 newDog() 创建的 Dog 对象。
 *
 * @param dog 要释放的 Dog 对象。
 */
void deleteDog(Dog* dog) {
        free(dog);
}

/**
 * @brief 将 Dog 视为其内嵌的 Animal 基类对象。
 *
 * 由于 base 位于结构体起始处，这一步本质上是一次“向上转型”。
 *
 * @param dog 要转换的 Dog 对象。
 * @return 指向内嵌 Animal 基类对象的指针。
 */
Animal* dogAsAnimal(Dog* dog) {
        return &(dog->base);
}
```

`Dog` 的构造函数里同样有一行非常关键：

```c
dog->base.vtblptr = &dogVtbl;
```

这说明每个具体对象在初始化时，都要把自己的函数表挂到内嵌的 `Animal base` 上。否则，抽象层就不知道应该分发到哪个具体函数。

## 调用端：只依赖抽象接口

在 `main.c` 里，创建对象时仍然使用具体类型的构造函数：

```c
Cat* cat = newCat("Tom");
Dog* dog = newDog("Max");
```

但真正调用行为时，走的是抽象接口：

```c
animalDrink(catAsAnimal(cat));
animalDrink(dogAsAnimal(dog));

animalSpeak(catAsAnimal(cat));
animalSpeak(dogAsAnimal(dog));
```

调用端调用的是同一组函数：`animalDrink()` 和 `animalSpeak()`。不同的输出不是由 `main.c` 里的 `if` / `switch` 决定的，而是由对象内部的 `vtblptr` 决定的。

### main.c

```c
#include "cat.h"
#include "dog.h"

int main(void) {
        Cat* cat = newCat("Tom");
        Dog* dog = newDog("Max");

        animalDrink(catAsAnimal(cat));
        animalDrink(dogAsAnimal(dog));

        animalSpeak(catAsAnimal(cat));
        animalSpeak(dogAsAnimal(dog));

        deleteCat(cat);
        deleteDog(dog);

        return 0;
}
```

这里的 `catAsAnimal()` 和 `dogAsAnimal()` 可以理解为手动的“向上转型”：

```c
Animal* catAsAnimal(Cat* cat) {
        return &(cat->base);
}
```

它把具体对象中的 `Animal base` 取出来，让调用端可以用统一的 `Animal*` 处理不同类型的对象。

## 编译运行

为了方便构建，我使用了一个简单的 `Makefile`。

### Makefile

```makefile
CC := gcc
CPPFLAGS := -Iinclude
CFLAGS := -Wall -Wextra -std=c11 -g

TARGET := main
SRCS := main.c src/animal.c src/cat.c src/dog.c
OBJS := $(SRCS:.c=.o)
HEADERS := include/animal.h include/cat.h include/dog.h

.PHONY: all run clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.c $(HEADERS)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET) $(OBJS)
```

运行：

```sh
make run
```

输出示例：

```text
I am Tom. (Init a cat)
I am Max. (Init a dog)
miaow~ Tom drink water.
woof~ Max drink water.
miaow~ I am Tom, a cat.
woof~ I am Max, a dog.
```

从输出可以看到，同样的抽象调用：

```c
animalDrink(...)
animalSpeak(...)
```

在 `Cat` 对象上表现为 `miaow~`，在 `Dog` 对象上表现为 `woof~`。这就是通过函数指针表实现的多态效果。

## 这份代码里的几个设计点

### 1. 用不透明类型实现封装

在 `cat.h` 和 `dog.h` 中，只写：

```c
typedef struct Cat Cat;
typedef struct Dog Dog;
```

真正的结构体定义放在 `cat.c` 和 `dog.c` 中：

```c
struct Cat {
        Animal base;
};
```

这样调用端无法直接访问 `Cat` / `Dog` 的内部字段。它只能通过公开函数创建、销毁或转换对象。

这就是 C 中常见的封装方式：头文件暴露接口，源文件隐藏实现。

### 2. 用函数表表达抽象接口

`AnimalVtbl` 定义了一组“动物应该具备的行为”：

```c
struct AnimalVtbl {
        void (*speak)(Animal* self);
        void (*drink)(Animal* self);
};
```

它本身不关心具体怎么叫、怎么喝水。它只规定：具体类型必须提供 `speak` 和 `drink` 这两个函数。

因此，`AnimalVtbl` 就是这份代码里的抽象接口。

### 3. 用内嵌 base 模拟继承

每个具体类型都把 `Animal` 嵌入到自己的结构体中：

```c
struct Cat {
        Animal base;
};

struct Dog {
        Animal base;
};
```

这让 `Cat` 和 `Dog` 都拥有一份公共的 `Animal` 数据，包括 `name` 和 `vtblptr`。

这种方式不是 C 语言层面的继承，而是一种工程上的组合约定：具体对象内部包含一个公共基类对象，外部通过这个公共基类对象进行统一操作。

### 4. 用 vtblptr 实现运行时分发

对象初始化时会保存自己的函数表：

```c
cat->base.vtblptr = &catVtbl;
dog->base.vtblptr = &dogVtbl;
```

抽象接口调用时再通过这个函数表分发：

```c
self->vtblptr->drink(self);
self->vtblptr->speak(self);
```

所以，`animalDrink()` 不需要知道 `self` 来自猫还是狗。它只要相信 `self->vtblptr` 已经指向了正确的函数表即可。

### 5. 新类型可以按同样模式扩展

如果以后要增加一个 `Bird`，大致只需要：

1. 定义 `Bird` 的公开接口，例如 `bird.h`。
2. 在 `bird.c` 中定义 `struct Bird { Animal base; };`。
3. 实现 `birdSpeak()` 和 `birdDrink()`。
4. 定义 `birdVtbl`。
5. 在 `newBird()` 中设置 `bird->base.vtblptr = &birdVtbl;`。
6. 提供 `birdAsAnimal()` 返回 `&(bird->base)`。

这样，调用端依旧可以通过 `animalSpeak()` 和 `animalDrink()` 使用它。

## 小结

这份代码展示的是一种朴素但非常重要的 C 语言抽象技巧：

- `struct` 负责组织对象数据。
- `.h` 和 `.c` 的边界负责隐藏实现细节。
- 函数指针负责描述可替换的行为。
- 函数表负责把一组行为打包成接口。
- 对象中的函数表指针负责在运行时选择具体实现。

用一句话概括就是：

> 在 C 语言里，可以通过“内嵌公共基类结构体 + 函数指针表 + 公开分发函数”的组合，手动实现类似面向对象语言中的封装、抽象和多态。

这种写法不会把 C 变成 C++，也不会自动提供类型检查、继承层次管理或析构链。它更像是一种清晰的工程约定：只要对象按约定初始化好自己的函数表，调用端就能用统一接口处理不同类型的对象。

## 下一章预告

这一章里，`Cat` 和 `Dog` 都把 `Animal base` 放在了结构体的第一个成员位置，因此 `Cat*` / `Dog*` 和 `&obj->base` 的地址数值刚好一致，很多转换看起来都很“自然”。

但真实工程里，派生类往往会加入自己的私有字段。只要 `Animal base` 不再位于结构体起始处，传进抽象接口的 `Animal*` 就不再等于完整对象的起始地址，直接强转回 `Cat*` 或 `Dog*` 就会出错。

下一章会继续讲这个问题：当派生类中的基类成员不再位于首位时，如何根据成员指针反推出外层对象地址，也就是 `container_of` 这类写法为什么能成立。

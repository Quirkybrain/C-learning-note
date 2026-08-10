# 从多态调用到简单析构：C 语言里的对象生命周期管理

- 作者：Quirkybrain
- GitHub 仓库：[Quirkybrain/C-learning-note](https://github.com/Quirkybrain/C-learning-note)

`001-c-polymorphism-with-vtable` 用 `AnimalVtbl` 把 `speak` 和 `drink` 做成了统一接口，说明 C 里也可以用结构体和函数指针模拟“抽象 + 多态”。

`002-c-container-of` 又补上了一个更底层但很关键的能力：当 `Animal base` 不再位于具体结构体第一个成员时，如何从 `Animal*` 安全地找回真正的 `Cat*` 或 `Dog*`。

这一章继续把前两章的思路往前推一步，填上前面刻意留下的一个坑：

```text
行为可以多态调用，那对象的释放能不能也走多态接口？
```

答案是可以，但前提是我们已经有了 002 的 `container_of()`。因为只有先能从 `Animal*` 反推出完整对象地址，具体类型才知道应该把哪一块 `malloc()` 出来的内存正确 `free()` 掉。

所以，003 的核心主题不是“复杂析构体系”，而是更朴素的一步：

- 把“释放对象本体”也纳入虚表。
- 调用端只面向 `Animal*` 管理生命周期。
- 具体类型自己负责释放正确的完整对象地址。

这是一种简单的析构方式，也是后续继续做析构链的起点。

## 构建与运行

当前工程仍然按 `include/` 和 `src/` 目录组织。由于 `container_of()` 使用了 GNU C 扩展里的 `typeof` 和 statement expression，Makefile 继续使用 `-std=gnu11`。

```bash
make
make run
```

运行结果：

```text
I am Tom. (Init a cat)
I am Max. (Init a dog)
miaow~ Tom drink water.
woof~ Max drink water.
miaow~ I am Tom, a cat, with 9 lives.
woof~ I am Max, a dog.
I am Tom. (destroy a cat)
I am Max. (destroy a dog)
```

前半段还是 001、002 已经有的多态行为调用；最后是 003 新加入的“多态销毁”。

## 这一章相对于前两章做了什么

可以把三个目录连起来看：

- `001` 解决的是“怎么多态调用行为”。
- `002` 解决的是“怎么从基类成员指针找回完整对象”。
- `003` 解决的是“怎么只拿着 `Animal*` 也能正确释放对象”。

如果没有 002，`Cat` 这种布局：

```c
struct Cat {
        int lives;
        Animal base;
};
```

在销毁时就会遇到问题。

调用端持有的是：

```c
Animal* animal = catAsAnimal(cat);
```

这个 `animal` 指向的是 `cat->base`，不是 `malloc()` 返回的 `Cat*` 起始地址。也就是说，不能简单写：

```c
free(animal);
```

因为这并不是当初 `malloc()` 返回的地址。真正该释放的是整个 `Cat` 对象地址，而不是中间成员 `base` 的地址。

这也是为什么 003 必须建立在 002 之后：先能 `container_of()`，再谈统一销毁。

## 抽象层：给虚表增加 destroy

003 中，`AnimalVtbl` 从“行为表”变成了一张更完整的“对象协议表”：

```c
struct AnimalVtbl {
        void (*speak)(Animal* self);
        void (*drink)(Animal* self);
        void (*destroy)(Animal** self);
};
```

前两个槽位负责多态行为分发，第三个槽位负责多态销毁。

这样设计之后，抽象层就可以提供统一入口：

```c
void destroyAnimal(Animal** self) {
        (*self)->vtblptr->destroy(self);
}
```

这段代码很像前面已经有的：

```c
self->vtblptr->speak(self);
self->vtblptr->drink(self);
```

本质上没有变，还是“抽象层分发，具体类型实现”。不同的只是：`destroy` 涉及生命周期结束，所以参数不再是 `Animal*`，而是 `Animal**`。

## 为什么 destroy 要接收 Animal**

这部分是本章最容易忽略、但很值得单独记一下的点。

如果 `destroy` 只写成：

```c
void (*destroy)(Animal* self);
```

那么具体类型确实可以在内部 `free()` 对象，但调用方手里保存的那个指针值不会变。

如果统一销毁入口也只接收 `Animal*`，调用方很可能会写成：

```c
// 错误示例
Animal* animal = catAsAnimal(cat);
destroyAnimal(animal);
```

释放之后，`animal` 变量里仍然是原来的地址，只不过这块地址已经失效了。这个指针就成了悬空指针。

003 改成：

```c
void (*destroy)(Animal** self);
```

这样具体类型在释放后就可以顺手写：

```c
*self = NULL;
```

调用方持有的那个抽象指针会被清空，于是“对象已经死了”这件事会更明确一些。

要注意，这只能清空当前传进去的那个指针变量：

- 如果程序里还有别名也指向同一个对象，它们不会自动变成 `NULL`。
- 所以这不是完整的生命周期安全机制。
- 但对这个示例来说，它已经足够展示“析构接口为什么需要双重指针”。

## Cat：简单析构的完整路径

`Cat` 在 003 里仍然保留了 002 的结构布局：

```c
struct Cat {
        int lives;
        Animal base;
};
```

这里故意让 `base` 不在首位，是为了继续强调一件事：

```text
Animal* 指向的是成员，不一定等于完整对象起始地址。
```

因此 `catSpeak()` 里要用 `container_of()` 找回 `Cat*`：

```c
static void catSpeak(Animal* self) {
        Cat* cat = container_of(self, Cat, base);
        printf("miaow~ I am %s, a cat, with %d lives.\n", self->name, cat->lives);
}
```

新的重点是 `destroyCat()`：

```c
static void destroyCat(Animal** self) {
        Cat* cat = container_of(*self, Cat, base);
        printf("I am %s. (destroy a cat)\n", (*self)->name);
        free(cat);
        *self = NULL;
}
```

它做了三件事：

1. 先从 `Animal*` 找回完整的 `Cat*`。
2. 释放真正由 `malloc(sizeof(Cat))` 得到的对象地址。
3. 把调用方传进来的那个 `Animal*` 置空。

这就是本章所谓“简单析构”的核心：先准确回到完整对象，再把对象本体释放掉。

## Dog：当前即使能直接转，也统一走同一套路

`Dog` 的布局是：

```c
struct Dog {
        Animal base;
};
```

因为 `base` 仍然是第一个成员，所以在当前版本里，`Dog*` 和 `&dog->base` 的地址数值相同。

但 `destroyDog()` 仍然统一写成：

```c
static void destroyDog(Animal** self) {
        Dog* dog = container_of(*self, Dog, base);
        printf("I am %s. (destroy a dog)\n", (*self)->name);
        free(dog);
        *self = NULL;
}
```

这样做的意义不是“Dog 现在必须这样”，而是让销毁规则不依赖当前字段顺序：

- 今天 `Dog` 没有私有字段。
- 明天它完全可以长成 `struct Dog { int age; Animal base; };`
- 只要依旧通过 `container_of(*self, Dog, base)` 恢复完整对象，析构路径就不用改思路。

换句话说，003 想传达的不只是“代码能跑”，而是“对象销毁规则和对象布局解耦”。

## 调用端：进入抽象世界后，调用和销毁都只面向 Animal

`main.c` 的结构非常能体现这一章的重点：

```c
Cat* cat = newCat("Tom");
Dog* dog = newDog("Max");

Animal* animals[2] = {
        catAsAnimal(cat),
        dogAsAnimal(dog)
};
```

创建对象时，调用端当然还得知道自己创建的是 `Cat` 还是 `Dog`。因为“创建什么类型”本来就是一个具体决策。

但一旦对象被放进 `Animal*` 抽象视图里：

```c
for (int i = 0; i < 2; ++i) {
        animalDrink(animals[i]);
}

for (int i = 0; i < 2; ++i) {
        animalSpeak(animals[i]);
}

for (int i = 0; i < 2; ++i) {
        destroyAnimal(&animals[i]);
}
```

后面的行为调用和生命周期结束，就都不需要再区分具体类型了。

这正是 003 填上的那部分空白：

```text
对象的“使用”可以抽象，
对象的“结束生命周期”也可以抽象。
```

## 这一章解决了什么，还没解决什么

003 已经解决的问题：

- 抽象层只持有 `Animal*` 时，也能把销毁分发给正确的具体类型。
- 具体类型可以释放正确的完整对象地址，而不是错误地 `free(base)`。
- 调用方可以用统一接口结束对象生命周期。
- 调用方传入的那个 `Animal*` 变量会在析构后被置为 `NULL`。

但这一章故意没有展开的内容也很重要：

- 现在释放的是“对象本体”。
- 如果具体类型内部还有自己额外申请的堆内存，这一章还没有形成完整的释放链。
- 也就是说，当前的 `destroyCat()` / `destroyDog()` 更像一个简单版本的终点释放，而不是一套完整的分层析构体系。

举个未来会遇到的例子。如果以后 `Cat` 里新增：

```c
struct Cat {
        char* favoriteFood;
        int lives;
        Animal base;
};
```

并且 `favoriteFood` 是通过 `malloc()`、`strdup()` 之类动态申请出来的，那么析构时就不能只 `free(cat)`。否则对象本体虽然释放了，`favoriteFood` 指向的那块堆内存却还没处理好。

所以，本章先只讲“对象本体的简单析构”，不急着把所有析构层级一次讲完。

## 小结

001 让我们有了多态调用，002 让我们能够从 `Animal*` 找回完整对象，003 则把这两个能力组合起来，补上了前面暂时没管理的生命周期部分。

从接口角度看，`AnimalVtbl` 不再只描述“这个对象会做什么”，也开始描述“这个对象应该如何结束自己”。

从实现角度看，003 的关键不是 `free()` 本身，而是：

- 先通过 `container_of()` 回到正确的完整对象地址。
- 再由具体类型负责释放。
- 最后把调用方当前持有的抽象指针清空。

这已经足够构成一个清晰、简单、可运行的析构模型。

## 下一章预告

004 会在这一章的基础上继续往前走，把“简单析构”扩展成“析构链”。

到那时，重点就不再只是：

```c
free(完整对象);
```

而会变成：

- 先释放派生类自己通过 `malloc()`、`strdup()` 等方式申请的成员资源。
- 再按约定把销毁动作一层层串起来。
- 最后再释放最外层对象本体。

也就是说，003 解决的是“谁来释放这个对象”，004 要继续解决的是“对象内部那些额外申请出来的资源，该按什么顺序一起释放掉”。

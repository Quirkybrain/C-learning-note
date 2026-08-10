# 从简单析构到析构链：C 语言里对象内部资源的释放顺序

- 作者：Quirkybrain
- GitHub 仓库：[Quirkybrain/C-learning-note](https://github.com/Quirkybrain/C-learning-note)

`001-c-polymorphism-with-vtable` 先用 `AnimalVtbl` 做出了多态调用：调用端只拿着 `Animal*`，真正执行的是 `catSpeak()` 还是 `dogSpeak()`，由对象内部的函数表决定。

`002-c-container-of` 补上了一个关键能力：当 `Animal base` 不在结构体第一个成员时，具体实现仍然可以从 `Animal*` 安全地找回完整的 `Cat*` 或 `Dog*`。

`003-c-object-lifetime-management` 又往前走了一步：把“释放对象本体”也放进虚表。调用端不需要判断对象到底是 `Cat` 还是 `Dog`，只需要调用统一的 `destroyAnimal()`。

但是 003 还留下了一个新的问题：

```text
如果具体对象内部还有自己 malloc() 出来的成员资源，
只 free 对象本体够不够？
```

答案是不够。

这一章的 `Dog` 新增了一个堆分配的 `foodName` 成员。这样一来，销毁 `Dog` 时就不能只做：

```c
free(dog);
```

因为 `dog->foodName` 指向的那块堆内存也需要释放。于是 004 的重点变成了：

- 先清理具体类型自己额外持有的资源。
- 再释放完整对象本体。
- 这个顺序由抽象层统一调度，而不是每个类型随手写一团 `free()`。

这就是本章所说的“析构链”：销毁不再只是一个终点动作，而是一个有顺序的过程。

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
woof~ I am Max, a dog, like to eat bone.
(destroy Cat's member if have)
I am Tom. (destroy a cat)
(destroy Dog's foodName member: bone.)
I am Max. (destroy a dog)
```

前半段还是行为多态调用。后半段可以看到，`Cat` 和 `Dog` 都先进入 `cleanUp` 阶段，再进入 `release` 阶段。

## 这一章相对于 003 改了什么

003 里的虚表是这样：

```c
struct AnimalVtbl {
        void (*speak)(Animal* self);
        void (*drink)(Animal* self);
        void (*destroy)(Animal** self);
};
```

也就是说，具体类型只需要实现一个 `destroy` 函数。这个函数既可以释放对象内部资源，也可以释放对象本体。

004 把这个单一动作拆成两个槽位：

```c
struct AnimalVtbl {
        void (*speak)(Animal* self);
        void (*drink)(Animal* self);
        void (*cleanUp)(Animal* self);
        void (*release)(Animal* self);
};
```

这两个新槽位的分工很明确：

- `cleanUp`：清理对象内部额外持有的资源，但不释放对象本体。
- `release`：释放完整对象本体。

于是销毁流程从：

```text
destroyAnimal()
    -> 具体类型 destroy()
        -> free(各种资源)
        -> free(对象本体)
```

变成了：

```text
destroyAnimal()
    -> 具体类型 cleanUp()
        -> free(成员资源)
    -> 具体类型 release()
        -> free(对象本体)
    -> 把调用方持有的 Animal* 置空
```

这个变化看起来只是多拆了一个函数，但它表达了一个更重要的设计意图：

```text
资源清理和对象释放不是同一件事。
```

## 抽象层：destroyAnimal 负责固定销毁顺序

003 里，`destroyAnimal()` 只是把调用转发给具体类型的 `destroy`：

```c
void destroyAnimal(Animal** self) {
        (*self)->vtblptr->destroy(self);
}
```

004 里，抽象层开始承担“销毁顺序”的责任：

```c
void destroyAnimal(Animal** self) {
        (*self)->vtblptr->cleanUp(*self);
        (*self)->vtblptr->release(*self);
        *self = NULL;
}
```

这里有一个小细节很重要：`destroyAnimal()` 仍然接收 `Animal**`，但虚表里的 `cleanUp` 和 `release` 接收的是 `Animal*`。

原因是：

- `destroyAnimal()` 需要把调用方手里的指针置为 `NULL`，所以它需要 `Animal**`。
- `cleanUp()` 只需要访问对象内容并清理资源，所以 `Animal*` 足够。
- `release()` 只需要从 `Animal*` 找回完整对象并释放它，所以 `Animal*` 也足够。

也就是说，双重指针只留在最外层统一入口里。具体类型的析构阶段不再负责修改调用方变量，它们只处理对象本身。

## Dog：新增一个需要单独清理的堆成员

003 里的 `Dog` 很简单：

```c
struct Dog {
        Animal base;
};
```

对象里没有额外资源，所以释放完整对象本体就够了。

004 里给 `Dog` 增加了一个私有成员：

```c
struct Dog {
        char* foodName;
        Animal base;
};
```

`foodName` 是一块单独申请出来的堆内存，不属于 `Dog` 结构体本体那一块 `malloc(sizeof(Dog))`。因此销毁时必须分两步：

```text
先 free(dog->foodName)
再 free(dog)
```

如果直接 `free(dog)`，`Dog` 对象本体确实被释放了，但 `foodName` 指向的那块内存就再也找不到了，这就是内存泄漏。

## Dog 初始化：对象本体和成员资源是两次分配

`newDog()` 负责分配对象本体：

```c
Dog* newDog(const char* name, const char* foodName) {
        Dog* dog = (Dog*) malloc (sizeof(Dog));
        if (dog == NULL) return NULL;

        dog->base.vtblptr = &dogVtbl;
        dogInit(dog, name, foodName);

        return dog;
}
```

真正初始化 `foodName` 的动作放在 `dogInit()` 里：

```c
static void dogInit(Dog* self, const char* name, const char* food) {
        strncpy(self->base.name, name, MAX_NAME_LEN - 1);
        self->base.name[MAX_NAME_LEN-1] = 0;

        self->foodName = (char*) calloc (MAX_DOG_FOOD_NAME, sizeof(char));
        strncpy(self->foodName, food, MAX_DOG_FOOD_NAME-1);
        self->foodName[MAX_DOG_FOOD_NAME-1] = 0;

        printf("I am %s. (Init a dog)\n", self->base.name);
}
```

这里有两层生命周期：

- `Dog* dog` 来自 `malloc(sizeof(Dog))`。
- `dog->foodName` 来自 `calloc(MAX_DOG_FOOD_NAME, sizeof(char))`。

既然创建时有两次分配，销毁时也应该有对应的两次释放。

这正是 004 相比 003 多出来的点：一个对象不一定只对应一块堆内存。对象本体里面还可能“拥有”其他资源。

## Dog 行为：访问私有字段仍然要靠 container_of

因为 `Dog` 现在长这样：

```c
struct Dog {
        char* foodName;
        Animal base;
};
```

`Animal base` 已经不在结构体第一个成员位置。调用端拿到的 `Animal*` 指向的是 `dog->base`，不是 `Dog` 对象起点。

所以 `dogSpeak()` 不能把 `Animal*` 直接强转成 `Dog*`，而要继续使用 002 引入的 `container_of()`：

```c
static void dogSpeak(Animal* self) {
        Dog* dog = container_of(self, Dog, base);
        printf("woof~ I am %s, a dog, like to eat %s.\n", self->name, dog->foodName);
}
```

这说明 `container_of()` 不只是服务于 `Cat`。只要具体类型的 `base` 不在首位，或者我们不想把对象布局绑定到“base 必须放第一个”这个约定上，就应该用同一套方式恢复完整对象。

## Dog 的两阶段销毁

`Dog` 的第一阶段是 `cleanUpDog()`：

```c
static void cleanUpDog(Animal* self) {
        Dog* dog = container_of(self, Dog, base);
        printf("(destroy Dog's foodName member: %s.)\n", dog->foodName);
        free(dog->foodName);
}
```

它只做一件事：释放 `Dog` 自己额外申请出来的 `foodName`。

注意，它不释放 `dog` 本体。这样 `destroyAnimal()` 在进入下一阶段时，`self` 仍然是有效的，`releaseDog()` 还能继续从 `Animal*` 找回完整对象：

```c
static void releaseDog(Animal* self) {
        Dog* dog = container_of(self, Dog, base);
        printf("I am %s. (destroy a dog)\n", self->name);
        free(dog);
}
```

于是完整顺序就是：

```text
destroyAnimal(&animal)
    -> cleanUpDog(animal)
        -> free(dog->foodName)
    -> releaseDog(animal)
        -> free(dog)
    -> animal = NULL
```

这个顺序不能反过来。如果先 `free(dog)`，再去访问 `dog->foodName`，那就是释放对象后继续访问对象内部字段，属于未定义行为。

## Cat：没有私有堆资源，也要遵守同一协议

`Cat` 当前没有像 `Dog` 一样新增堆成员，它的结构仍然是：

```c
struct Cat {
        int lives;
        Animal base;
};
```

所以 `Cat` 的 `cleanUp` 阶段暂时没有真正要释放的资源：

```c
static void cleanUpCat(Animal* self) {
        (void)self;
        printf("(destroy Cat's member if have)\n");
        return;
}
```

但它仍然在虚表里提供了 `cleanUp`：

```c
static const AnimalVtbl catVtbl = {
        .speak = catSpeak,
        .drink = catDrink,
        .release = releaseCat,
        .cleanUp = cleanUpCat
};
```

这样做的意义是让所有具体类型都遵守同一套销毁协议。

`Cat` 现在没有私有资源，所以 `cleanUpCat()` 为空；以后如果给 `Cat` 增加：

```c
char* favoriteFood;
```

那么只需要把释放逻辑补进 `cleanUpCat()`，`destroyAnimal()` 的整体流程不需要再改。

`Cat` 的第二阶段仍然负责释放完整对象本体：

```c
static void releaseCat(Animal* self) {
        Cat* cat = container_of(self, Cat, base);
        printf("I am %s. (destroy a cat)\n", self->name);
        free(cat);
}
```

## 调用端：销毁接口没有变复杂

虽然内部从一个 `destroy` 拆成了 `cleanUp + release`，但调用端并没有变复杂。

`main.c` 仍然只需要把对象放进 `Animal*` 数组：

```c
Cat* cat = newCat("Tom");
Dog* dog = newDog("Max", "bone");

Animal* animals[2] = {
        catAsAnimal(cat),
        dogAsAnimal(dog)
};
```

然后统一调用：

```c
for (int i = 0; i < 2; ++i) {
        destroyAnimal(&animals[i]);
}
```

调用端不知道 `Dog` 有 `foodName`，也不知道 `Cat` 当前没有额外资源。它只知道一件事：

```text
这个 Animal* 结束生命周期了。
```

具体该清理哪些资源、对象本体该怎么释放，都由对象自己的虚表决定。

## 为什么不把所有 free 都写进一个 destroy 函数

003 的写法其实也可以继续扩展成这样：

```c
static void destroyDog(Animal** self) {
        Dog* dog = container_of(*self, Dog, base);
        free(dog->foodName);
        free(dog);
        *self = NULL;
}
```

这段代码不是不能工作。对于当前这个小例子，它甚至更短。

但它有一个问题：所有事情都混在了同一个函数里。

当对象变复杂以后，这种写法会让 `destroy` 同时承担很多职责：

- 从抽象指针恢复完整对象。
- 清理具体类型自己的堆成员。
- 释放对象本体。
- 修改调用方指针。
- 维护这些动作之间的顺序。

004 把这些职责拆开之后，边界更清楚：

- `destroyAnimal()` 负责统一入口和销毁顺序。
- `cleanUp()` 负责对象内部资源清理。
- `release()` 负责对象本体释放。
- `container_of()` 负责从基类成员指针恢复完整对象。

这样做的好处不是“代码行数变少”，而是生命周期规则变得更稳定。

以后某个类型新增私有资源时，通常只需要改自己的 `cleanUp()`。对象本体释放仍然放在 `release()`，调用端也不用知道这件事。

## 这种设计比单个 destroy 好在哪里

第一，释放顺序更明确。

对象内部资源必须在对象本体释放之前清理。把 `cleanUp` 放在 `release` 前面，顺序直接写在抽象层里，读代码时不用到每个类型的 `destroy` 里猜它到底先做什么。

第二，职责更单一。

`cleanUpDog()` 只关心 `foodName`，`releaseDog()` 只关心 `free(dog)`。这比把所有释放逻辑塞进一个函数里更容易检查，也更容易给别人讲清楚。

第三，具体类型更容易演进。

今天 `Dog` 只有一个 `foodName`。明天它可能有更多资源：

```c
char* foodName;
char* toyName;
int* trainingScores;
```

这些都可以继续放进 `cleanUpDog()`。只要 `releaseDog()` 仍然最后释放对象本体，整体生命周期顺序就不会乱。

第四，调用端仍然保持抽象。

`main.c` 不需要因为 `Dog` 多了一个堆成员就改销毁逻辑。调用端依然只写：

```c
destroyAnimal(&animals[i]);
```

这说明抽象接口没有被具体类型的内部变化污染。

第五，它更接近底层工程里的习惯。

很多 C 工程会把“对象从系统里摘掉”“清理对象持有资源”“最后释放对象内存”拆成不同阶段。Linux 内核里也经常能看到类似的思想：通用层负责生命周期入口和顺序，具体类型在自己的回调里处理自己拥有的资源。

这里不需要把它理解成 C++ 那种语言自动帮你串起来的析构函数。它更像一种手写约定：

```text
具体类型先清理自己拥有的东西，
然后统一释放对象本体。
```

## 这一章解决了什么，还没解决什么

004 已经解决的问题：

- `Dog` 可以拥有自己单独申请的堆成员。
- 销毁时会先释放 `dog->foodName`，再释放 `dog` 本体。
- `Cat` 和 `Dog` 都遵守同一套 `cleanUp + release` 协议。
- 调用端仍然只面向 `Animal*` 和 `destroyAnimal()`。
- `Animal*` 在销毁完成后仍然会被置为 `NULL`。

但这个示例仍然保留了一些问题没有解决：

- 没有做引用计数
- 没有处理多个指针别名同时指向同一个对象的情况。

这些都可以继续作为后续章节扩展。当前这一章先把最核心的顺序讲清楚：

```text
先释放成员资源，再释放对象本体。
```

## 小结

001 让对象可以多态调用行为。

002 让具体实现可以从 `Animal*` 找回完整对象。

003 让调用端可以通过统一接口释放对象本体。

004 则继续补上对象内部资源的释放顺序：对象不一定只拥有自己这一块内存，它还可能拥有其他堆资源。销毁时应该先清理这些资源，再释放对象本体。

从 003 到 004，最关键的变化不是多了一个 `foodName` 字段，而是销毁协议从：

```text
destroy
```

变成了：

```text
cleanUp -> release
```

这让“析构”从一个简单的 `free()` 动作，变成了一个可以继续扩展、可以分层讲清楚的生命周期过程。

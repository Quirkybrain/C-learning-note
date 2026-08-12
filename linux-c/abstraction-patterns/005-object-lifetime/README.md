# 从直接销毁到引用计数：C 语言里的共享对象生命周期

- 作者：Quirkybrain
- GitHub 仓库：[Quirkybrain/C-learning-note](https://github.com/Quirkybrain/C-learning-note)

在前面的几章里，我逐步搭起了一套简单的 C 语言对象模型：用函数表组织公共行为，用 `container_of()` 找回完整对象，再通过 `cleanUp -> release` 清理成员资源并释放对象本体。

004 已经讲清楚了对象应该怎样销毁，但调用端使用的是 `destroyAnimal()`：

```text
现在就销毁这个对象。
```

当同一个对象被多个 owner 共享时，一个 owner 不再使用对象，并不代表其他 owner 也已经离开。因此这一章给 `Animal` 增加引用计数，用 `get/put` 管理共享生命周期：

```text
get：新增一个 owner
put：当前 owner 离开
引用计数归零：真正销毁对象
```

## 构建与运行

```bash
make
make run
```

运行结果：

```text
============================================================
  [STEP 1] CREATE A DOG AND SHARE ITS OWNERSHIP
============================================================
[CREATE] Create Dog Max and expose it through Animal.

I am Max. (Init a dog)
[GET] Create a second owner: sharedAnimal.
woof~ I am Max, a dog, like to eat bone.
woof~ I am Max, a dog, like to eat bone.

============================================================
  [STEP 2] RELEASE OWNER: animal
============================================================
[PUT] Release animal, then verify that sharedAnimal keeps the object alive.

[OWNER RELEASED] animal is now NULL.
woof~ I am Max, a dog, like to eat bone.
[OBJECT ALIVE] sharedAnimal still owns the dog.

============================================================
  [STEP 3] RELEASE THE FINAL OWNER: sharedAnimal
============================================================
[FINAL PUT] Releasing sharedAnimal will destroy the dog.

(destroy Dog's foodName member: bone.)
I am Max. (destroy a dog)

************************************************************
  [OBJECT DESTROYED] No owners remain.
************************************************************
```

第一次 `animalPut()` 只释放一个引用，对象仍然存活；第二次 `animalPut()` 让引用计数归零，随后触发最终销毁。

## 004 的直接销毁遇到了什么问题

004 中可以直接结束对象生命周期：

```c
Animal* animal = dogAsAnimal(dog);
Animal* sharedAnimal = animal;

destroyAnimal(&animal);
```

`animal` 和 `sharedAnimal` 原本指向同一个 Dog。调用 `destroyAnimal()` 后，Dog 的资源已经释放，但另一个 owner 还保存着原来的地址。

直接销毁只表达“立即释放对象”，无法表示“当前 owner 离开，但其他 owner 仍在使用对象”。所以 005 不再由某一个 owner 直接决定销毁时机，而是统计仍然存在的拥有引用。

## 在 Animal 中加入引用计数

005 在公共的 `Animal` 中增加 `refCount`：

```c
struct Animal {
        char name[MAX_NAME_LEN];
        const AnimalVtbl* vtblptr;
        unsigned int refCount;
};
```

共享生命周期是所有具体类型共同遵守的规则，所以引用计数放在公共对象中。抽象层只需要拿到 `Animal*`，就可以统一增加或释放引用。

对象创建完成时，调用者已经持有第一个引用，因此 Cat 和 Dog 都把初始计数设置为 1：

```c
self->base.refCount = 1;
```

```text
object        0 -> 1
```

## animalGet：取得一个新引用

`animalGet()` 用于增加一个 owner：

```c
Animal* animalGet(Animal* self) {
        assert(self->refCount > 0);
        ++self->refCount;

        return self;
}
```

调用方式：

```c
Animal* sharedAnimal = animalGet(animal);
```

返回值仍然指向同一个对象，但引用计数从 1 变成 2：

```text
animal         ──┐
                 ├──> Dog，refCount = 2
sharedAnimal   ──┘
```

每次 `animalGet()` 都取得一个新的拥有引用，并且最终需要对应一次 `animalPut()`。

## animalPut：释放一个引用

`animalPut()` 表示当前 owner 不再使用对象：

```c
void animalPut(Animal** self) {
        Animal* object = *self;
        *self = NULL;

        assert(object->refCount > 0);
        --object->refCount;

        if (object->refCount == 0) {
                object->vtblptr->cleanUp(object);
                object->vtblptr->release(object);
        }
}
```

它先保存对象地址并清空调用方指针，然后减少引用计数：

- 计数仍大于 0：还有 owner，对象继续存活。
- 计数等于 0：调用 `cleanUp` 和 `release`，结束对象生命周期。

这里继续使用 `Animal**`，是因为 `animalPut()` 需要把调用方持有的指针置为 `NULL`：

```c
animalPut(&animal);
```

## main 中的完整生命周期

`main()` 先创建 Dog，并在后续统一通过 `Animal*` 使用它：

```c
Dog* dog = newDog("Max", "bone");

Animal* animal = dogAsAnimal(dog);
dog = NULL;
```

此时对象拥有一个初始引用。随后创建第二个 owner：

```c
Animal* sharedAnimal = animalGet(animal);
```

两个 owner 都可以使用同一个对象：

```c
animalSpeak(animal);
animalSpeak(sharedAnimal);
```

接着依次释放两个引用：

```c
animalPut(&animal);
animalSpeak(sharedAnimal);

animalPut(&sharedAnimal);
```

完整的引用计数变化如下：

| 操作 | `refCount` | 结果 |
|---|---:|---|
| `newDog()` | 1 | 建立初始引用 |
| `animalGet(animal)` | 2 | `sharedAnimal` 成为第二个 owner |
| `animalPut(&animal)` | 1 | `animal` 离开，对象继续存活 |
| `animalPut(&sharedAnimal)` | 0 | 最后一个 owner 离开，销毁对象 |

最后一次 `put` 对 Dog 触发的流程是：

```text
animalPut(&sharedAnimal)
    -> refCount: 1 -> 0
    -> cleanUpDog()
        -> free(dog->foodName)
    -> releaseDog()
        -> free(dog)
```

004 和 005 的职责由此组合起来：

```text
引用计数决定何时销毁。
析构链决定如何销毁。
```

## 指针和引用不是同一件事

直接复制指针只会复制地址：

```c
Animal* sharedAnimal = animal;
```

它不会改变 `refCount`。要建立一个新的拥有引用，需要显式调用：

```c
Animal* sharedAnimal = animalGet(animal);
```

引用计数统计的是通过生命周期协议登记的 owner，而不是相同地址出现了多少次。基本规则是：

```text
创建得到的初始引用  -> 一次 put
每一次 get          -> 一次 put
```

这种 `get/put` 命名在 C 语言工程中很常见。读者也可以继续了解 Linux 的 `kref`、`kobject_get()/kobject_put()` 和 `get_device()/put_device()`。

## 小结

004 使用 `cleanUp -> release` 规定最终销毁顺序，005 使用引用计数决定进入析构链的时机：

```text
new
    -> refCount = 1
    -> get 增加 owner
    -> put 释放 owner
    -> refCount = 0
        -> cleanUp
        -> release
```

这一章的核心只有一句话：

```text
每个 owner 持有一个引用，每个引用对应一次 put，最后一个 put 结束对象生命周期。
```

## 下一章预告

006 将介绍侵入式链表：把链表节点直接嵌入对象结构体，再通过 `container_of()` 从节点恢复完整对象，从单个共享对象继续走向一组对象的组织与遍历。

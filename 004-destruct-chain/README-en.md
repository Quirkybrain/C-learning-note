# From Simple Destruction to a Destruction Chain: The Order of Releasing Internal Resources in C Objects

- Author: Quirkybrain
- GitHub repository: [Quirkybrain/C-learning-note](https://github.com/Quirkybrain/C-learning-note)

`001-c-polymorphism-with-vtable` first used `AnimalVtbl` to implement polymorphic calls: the caller only holds an `Animal*`, while whether `catSpeak()` or `dogSpeak()` is actually executed is decided by the function table inside the object.

`002-c-container-of` then added a key capability: when `Animal base` is not the first member of a structure, the concrete implementation can still safely recover the complete `Cat*` or `Dog*` from an `Animal*`.

`003-c-object-lifetime-management` went one step further: it also placed “releasing the object itself” into the virtual table. The caller no longer needs to check whether the object is a `Cat` or a `Dog`; it only needs to call the unified `destroyAnimal()` function.

However, 003 still leaves a new question:

```text
If a concrete object owns member resources allocated by malloc(),
is it enough to only free the object itself?
```

The answer is no.

In this chapter, `Dog` gains a new heap-allocated member: `foodName`. As a result, destroying a `Dog` can no longer be done with only:

```c
free(dog);
```

because the heap memory pointed to by `dog->foodName` must also be released. Therefore, the focus of 004 becomes:

- First clean up the extra resources owned by the concrete type.
- Then release the complete object itself.
- Let this order be coordinated by the abstraction layer instead of letting every type casually write its own group of `free()` calls.

This is what this chapter calls a “destruction chain”: destruction is no longer just a final action, but an ordered process.

## Build and Run

The current project is still organized by the `include/` and `src/` directories. Because `container_of()` uses GNU C extensions such as `typeof` and statement expressions, the Makefile continues to use `-std=gnu11`.

```bash
make
make run
```

Output:

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

The first half is still polymorphic behavior calls. In the second half, you can see that both `Cat` and `Dog` enter the `cleanUp` phase first, and then the `release` phase.

## What Changed Compared with 003?

In 003, the virtual table looked like this:

```c
struct AnimalVtbl {
        void (*speak)(Animal* self);
        void (*drink)(Animal* self);
        void (*destroy)(Animal** self);
};
```

In other words, a concrete type only needed to implement one `destroy` function. This function could release both the object’s internal resources and the object itself.

In 004, this single action is split into two slots:

```c
struct AnimalVtbl {
        void (*speak)(Animal* self);
        void (*drink)(Animal* self);
        void (*cleanUp)(Animal* self);
        void (*release)(Animal* self);
};
```

The responsibilities of these two new slots are clear:

- `cleanUp`: clean up extra resources owned by the object, but do not release the object itself.
- `release`: release the complete object itself.

So the destruction process changes from:

```text
destroyAnimal()
    -> concrete type destroy()
        -> free(various resources)
        -> free(the object itself)
```

to:

```text
destroyAnimal()
    -> concrete type cleanUp()
        -> free(member resources)
    -> concrete type release()
        -> free(the object itself)
    -> set the caller's Animal* to NULL
```

This change may look like only one function was split into two, but it expresses a more important design intention:

```text
Resource cleanup and object release are not the same thing.
```

## The Abstraction Layer: destroyAnimal Controls the Destruction Order

In 003, `destroyAnimal()` simply forwarded the call to the concrete type’s `destroy` function:

```c
void destroyAnimal(Animal** self) {
        (*self)->vtblptr->destroy(self);
}
```

In 004, the abstraction layer begins to take responsibility for the “destruction order”:

```c
void destroyAnimal(Animal** self) {
        (*self)->vtblptr->cleanUp(*self);
        (*self)->vtblptr->release(*self);
        *self = NULL;
}
```

There is an important detail here: `destroyAnimal()` still receives an `Animal**`, but the `cleanUp` and `release` functions in the virtual table receive an `Animal*`.

The reason is:

- `destroyAnimal()` needs to set the caller’s pointer to `NULL`, so it needs an `Animal**`.
- `cleanUp()` only needs to access the object content and clean up resources, so `Animal*` is enough.
- `release()` only needs to recover the complete object from `Animal*` and release it, so `Animal*` is also enough.

That is, the double pointer is kept only at the outermost unified entry point. The concrete destruction phases no longer need to modify the caller’s variable; they only handle the object itself.

## Dog: Adding a Heap Member That Must Be Cleaned Up Separately

In 003, `Dog` was very simple:

```c
struct Dog {
        Animal base;
};
```

The object had no extra resources, so releasing the complete object itself was enough.

In 004, `Dog` gets a private member:

```c
struct Dog {
        char* foodName;
        Animal base;
};
```

`foodName` is a separately allocated heap block. It is not part of the memory block allocated by `malloc(sizeof(Dog))`. Therefore, destruction must happen in two steps:

```text
First free(dog->foodName)
Then free(dog)
```

If you directly call `free(dog)`, the `Dog` object itself is indeed released, but the memory pointed to by `foodName` can no longer be found. That is a memory leak.

## Dog Initialization: The Object and Its Member Resource Are Allocated Separately

`newDog()` is responsible for allocating the object itself:

```c
Dog* newDog(const char* name, const char* foodName) {
        Dog* dog = (Dog*) malloc (sizeof(Dog));
        if (dog == NULL) return NULL;

        dog->base.vtblptr = &dogVtbl;
        dogInit(dog, name, foodName);

        return dog;
}
```

The actual initialization of `foodName` is placed in `dogInit()`:

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

There are two layers of lifetime here:

- `Dog* dog` comes from `malloc(sizeof(Dog))`.
- `dog->foodName` comes from `calloc(MAX_DOG_FOOD_NAME, sizeof(char))`.

Since creation involves two allocations, destruction should also involve two corresponding releases.

This is exactly the new point in 004 compared with 003: one object does not necessarily correspond to only one heap block. The object itself may also “own” other resources.

## Dog Behavior: Accessing Private Fields Still Depends on container_of

Because `Dog` now looks like this:

```c
struct Dog {
        char* foodName;
        Animal base;
};
```

`Animal base` is no longer the first member of the structure. The `Animal*` held by the caller points to `dog->base`, not to the beginning of the `Dog` object.

So `dogSpeak()` cannot directly cast `Animal*` to `Dog*`; it must continue to use the `container_of()` introduced in 002:

```c
static void dogSpeak(Animal* self) {
        Dog* dog = container_of(self, Dog, base);
        printf("woof~ I am %s, a dog, like to eat %s.\n", self->name, dog->foodName);
}
```

This shows that `container_of()` is not only useful for `Cat`. As long as the concrete type’s `base` is not placed first, or as long as we do not want to bind the object layout to the rule that “base must be the first member,” we should use the same mechanism to recover the complete object.

## Dog’s Two-Phase Destruction

The first phase for `Dog` is `cleanUpDog()`:

```c
static void cleanUpDog(Animal* self) {
        Dog* dog = container_of(self, Dog, base);
        printf("(destroy Dog's foodName member: %s.)\n", dog->foodName);
        free(dog->foodName);
}
```

It does only one thing: release the `foodName` that `Dog` allocated separately.

Notice that it does not release the `dog` object itself. This way, when `destroyAnimal()` enters the next phase, `self` is still valid, and `releaseDog()` can still recover the complete object from `Animal*`:

```c
static void releaseDog(Animal* self) {
        Dog* dog = container_of(self, Dog, base);
        printf("I am %s. (destroy a dog)\n", self->name);
        free(dog);
}
```

Therefore, the complete order is:

```text
destroyAnimal(&animal)
    -> cleanUpDog(animal)
        -> free(dog->foodName)
    -> releaseDog(animal)
        -> free(dog)
    -> animal = NULL
```

This order cannot be reversed. If you call `free(dog)` first and then access `dog->foodName`, you are accessing an internal field of an already released object, which is undefined behavior.

## Cat: No Private Heap Resource, but Still Following the Same Protocol

At the moment, `Cat` has no heap member like `Dog`. Its structure is still:

```c
struct Cat {
        int lives;
        Animal base;
};
```

So `Cat`’s `cleanUp` phase does not really have any resource to release for now:

```c
static void cleanUpCat(Animal* self) {
        (void)self;
        printf("(destroy Cat's member if have)\n");
        return;
}
```

But it still provides `cleanUp` in the virtual table:

```c
static const AnimalVtbl catVtbl = {
        .speak = catSpeak,
        .drink = catDrink,
        .release = releaseCat,
        .cleanUp = cleanUpCat
};
```

The purpose is to make all concrete types follow the same destruction protocol.

`Cat` currently has no private resource, so `cleanUpCat()` is empty. If we later add:

```c
char* favoriteFood;
```

then we only need to add the release logic into `cleanUpCat()`. The overall process in `destroyAnimal()` does not need to change.

The second phase for `Cat` is still responsible for releasing the complete object itself:

```c
static void releaseCat(Animal* self) {
        Cat* cat = container_of(self, Cat, base);
        printf("I am %s. (destroy a cat)\n", self->name);
        free(cat);
}
```

## The Caller: The Destruction Interface Does Not Become More Complicated

Although the internal design changes from a single `destroy` function into `cleanUp + release`, the caller does not become more complicated.

`main.c` still only needs to put objects into an `Animal*` array:

```c
Cat* cat = newCat("Tom");
Dog* dog = newDog("Max", "bone");

Animal* animals[2] = {
        catAsAnimal(cat),
        dogAsAnimal(dog)
};
```

Then it calls the unified destruction function:

```c
for (int i = 0; i < 2; ++i) {
        destroyAnimal(&animals[i]);
}
```

The caller does not know that `Dog` has a `foodName`, and it also does not know that `Cat` currently has no extra resource. It only knows one thing:

```text
This Animal* has reached the end of its lifetime.
```

Which resources should be cleaned up, and how the object itself should be released, are decided by the object’s own virtual table.

## Why Not Put All free Calls into a Single destroy Function?

The design from 003 could also be extended like this:

```c
static void destroyDog(Animal** self) {
        Dog* dog = container_of(*self, Dog, base);
        free(dog->foodName);
        free(dog);
        *self = NULL;
}
```

This code is not wrong. For this small example, it is even shorter.

But it has a problem: everything is mixed into one function.

As the object becomes more complex, this style forces `destroy` to take on too many responsibilities at once:

- Recovering the complete object from the abstract pointer.
- Cleaning up heap members owned by the concrete type.
- Releasing the object itself.
- Modifying the caller’s pointer.
- Maintaining the order among all these actions.

After 004 splits these responsibilities apart, the boundaries become clearer:

- `destroyAnimal()` is responsible for the unified entry point and the destruction order.
- `cleanUp()` is responsible for cleaning up internal resources.
- `release()` is responsible for releasing the object itself.
- `container_of()` is responsible for recovering the complete object from a base-member pointer.

The benefit is not that the code becomes shorter. The benefit is that the lifetime rules become more stable.

In the future, when a concrete type gains a private resource, you usually only need to modify its own `cleanUp()`. Releasing the object itself remains in `release()`, and the caller does not need to know about the change.

## Why Is This Better Than a Single destroy Function?

First, the release order is clearer.

Internal resources must be cleaned up before the object itself is released. By placing `cleanUp` before `release`, the order is written directly in the abstraction layer. When reading the code, you no longer need to inspect every concrete type’s `destroy` function to guess what it does first.

Second, each responsibility is more focused.

`cleanUpDog()` only cares about `foodName`, while `releaseDog()` only cares about `free(dog)`. This is easier to inspect and easier to explain than putting all release logic into one function.

Third, concrete types become easier to evolve.

Today, `Dog` only has one `foodName`. Tomorrow, it may have more resources:

```c
char* foodName;
char* toyName;
int* trainingScores;
```

All of these can continue to be handled inside `cleanUpDog()`. As long as `releaseDog()` still releases the object itself at the end, the overall lifetime order remains stable.

Fourth, the caller remains abstract.

`main.c` does not need to change its destruction logic just because `Dog` gains another heap member. The caller still writes only:

```c
destroyAnimal(&animals[i]);
```

This shows that the abstraction interface has not been polluted by the internal changes of a concrete type.

Fifth, this is closer to habits used in low-level engineering.

Many C projects split “removing an object from the system,” “cleaning up resources owned by the object,” and “finally releasing the object memory” into different phases. Similar ideas can often be found in the Linux kernel as well: the generic layer controls the lifetime entry point and the order, while each concrete type handles the resources it owns through its own callbacks.

There is no need to understand this as a C++-style destructor automatically chained by the language. It is more like a manually written convention:

```text
The concrete type first cleans up what it owns,
and then the object itself is released in a unified way.
```

## What This Chapter Solves, and What It Does Not Solve Yet

Problems solved in 004:

- `Dog` can own a separately allocated heap member.
- During destruction, `dog->foodName` is released first, and then the `dog` object itself is released.
- Both `Cat` and `Dog` follow the same `cleanUp + release` protocol.
- The caller still only works with `Animal*` and `destroyAnimal()`.
- After destruction completes, the `Animal*` is still set to `NULL`.

However, this example still leaves some problems unresolved:

- No reference counting is implemented.
- It does not handle the case where multiple pointer aliases point to the same object.

These topics can be expanded in later chapters. For now, this chapter focuses on making the most essential order clear:

```text
Release member resources first, then release the object itself.
```

## Summary

001 allows objects to perform polymorphic behavior calls.

002 allows the concrete implementation to recover the complete object from an `Animal*`.

003 allows the caller to release the object itself through a unified interface.

004 continues by adding the release order for resources inside the object: an object does not necessarily own only the memory block of itself. It may also own other heap resources. During destruction, these resources should be cleaned up first, and then the object itself should be released.

From 003 to 004, the key change is not the addition of a `foodName` field, but the evolution of the destruction protocol from:

```text
destroy
```

to:

```text
cleanUp -> release
```

This turns “destruction” from a simple `free()` action into a lifetime process that can keep growing, remain layered, and be explained clearly.

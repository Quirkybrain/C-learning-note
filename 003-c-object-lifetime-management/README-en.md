# From Polymorphic Calls to Simple Destruction: Object Lifetime Management in C

- Author: Quirkybrain
- GitHub repository: [Quirkybrain/C-learning-note](https://github.com/Quirkybrain/C-learning-note)

In `001-c-polymorphism-with-vtable`, `AnimalVtbl` was used to turn `speak` and `drink` into a unified interface. It showed that C can also simulate “abstraction + polymorphism” with structures and function pointers.

In `002-c-container-of`, we added a lower-level but very important capability: when `Animal base` is no longer the first member of a concrete structure, how can we safely recover the real `Cat*` or `Dog*` from an `Animal*`?

This chapter takes the ideas from the previous two chapters one step further and fills in a gap that was deliberately left open:

```text
If behavior can be called polymorphically, can object destruction also go through a polymorphic interface?
```

The answer is yes, but only after we already have the `container_of()` mechanism from 002. Only when we can recover the complete object address from an `Animal*` can each concrete type know which block of memory allocated by `malloc()` should actually be passed to `free()`.

So the core topic of 003 is not a “complex destructor system.” It is a much simpler first step:

- Put “releasing the object itself” into the virtual table.
- Let the caller manage lifetime only through `Animal*`.
- Let each concrete type release the correct complete object address by itself.

This is a simple destruction model, and it is also the starting point for building a destructor chain later.

## Build and Run

The project is still organized with `include/` and `src/` directories. Since `container_of()` uses GNU C extensions such as `typeof` and statement expressions, the Makefile continues to use `-std=gnu11`.

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
woof~ I am Max, a dog.
I am Tom. (destroy a cat)
I am Max. (destroy a dog)
```

The first part is still the polymorphic behavior calls already shown in 001 and 002. The last part is the new “polymorphic destruction” added in 003.

## What This Chapter Adds Compared with the Previous Two

The three directories can be read as a continuous path:

- `001` answers: “How do we call behavior polymorphically?”
- `002` answers: “How do we recover the complete object from a base-member pointer?”
- `003` answers: “How do we correctly release an object when we only have an `Animal*`?”

Without 002, a layout like this for `Cat` would cause a problem during destruction:

```c
struct Cat {
        int lives;
        Animal base;
};
```

The caller holds:

```c
Animal* animal = catAsAnimal(cat);
```

This `animal` points to `cat->base`, not to the starting address of the `Cat*` returned by `malloc()`. In other words, we cannot simply write:

```c
free(animal);
```

because this is not the address originally returned by `malloc()`. The correct address to release is the address of the complete `Cat` object, not the address of the middle member `base`.

This is why 003 must be built on top of 002: first we need `container_of()`, then we can talk about unified destruction.

## The Abstraction Layer: Adding `destroy` to the Virtual Table

In 003, `AnimalVtbl` evolves from a “behavior table” into a more complete “object protocol table”:

```c
struct AnimalVtbl {
        void (*speak)(Animal* self);
        void (*drink)(Animal* self);
        void (*destroy)(Animal** self);
};
```

The first two slots are responsible for polymorphic behavior dispatch. The third slot is responsible for polymorphic destruction.

With this design, the abstraction layer can provide a unified entry point:

```c
void destroyAnimal(Animal** self) {
        (*self)->vtblptr->destroy(self);
}
```

This code is very similar to the previous calls:

```c
self->vtblptr->speak(self);
self->vtblptr->drink(self);
```

The essence is unchanged: the abstraction layer dispatches the call, and the concrete type provides the implementation. The only difference is that `destroy` involves the end of an object’s lifetime, so its parameter is no longer `Animal*`, but `Animal**`.

## Why `destroy` Receives `Animal**`

This is the easiest detail to overlook in this chapter, but it is worth noting separately.

If `destroy` were written like this:

```c
void (*destroy)(Animal* self);
```

then the concrete type could indeed call `free()` internally, but the pointer variable held by the caller would not change.

If the unified destruction entry also received only `Animal*`, the caller might write:

```c
// Incorrect example
Animal* animal = catAsAnimal(cat);
destroyAnimal(animal);
```

After the object is released, the `animal` variable would still contain the old address. That address would no longer be valid, so the pointer would become a dangling pointer.

In 003, it is changed to:

```c
void (*destroy)(Animal** self);
```

This allows the concrete type to write the following after releasing the object:

```c
*self = NULL;
```

The abstract pointer held by the caller is cleared, making it more explicit that “this object is already dead.”

One important thing to remember: this only clears the pointer variable passed into the function.

- If other aliases in the program still point to the same object, they will not automatically become `NULL`.
- So this is not a complete lifetime-safety mechanism.
- But for this example, it is enough to show why a destructor interface may need a double pointer.

## `Cat`: The Complete Path of Simple Destruction

In 003, `Cat` keeps the structure layout from 002:

```c
struct Cat {
        int lives;
        Animal base;
};
```

Here, `base` is intentionally not placed as the first member. This keeps emphasizing one important point:

```text
An Animal* points to a member, and it is not necessarily equal to the starting address of the complete object.
```

Therefore, `catSpeak()` needs to use `container_of()` to recover the `Cat*`:

```c
static void catSpeak(Animal* self) {
        Cat* cat = container_of(self, Cat, base);
        printf("miaow~ I am %s, a cat, with %d lives.\n", self->name, cat->lives);
}
```

The new focus is `destroyCat()`:

```c
static void destroyCat(Animal** self) {
        Cat* cat = container_of(*self, Cat, base);
        printf("I am %s. (destroy a cat)\n", (*self)->name);
        free(cat);
        *self = NULL;
}
```

It does three things:

1. Recover the complete `Cat*` from `Animal*`.
2. Release the real object address obtained from `malloc(sizeof(Cat))`.
3. Set the `Animal*` passed in by the caller to `NULL`.

This is the core of the “simple destruction” model in this chapter: first recover the correct complete object, then release the object itself.

## `Dog`: Even When Direct Casting Works Today, Use the Same Pattern

The layout of `Dog` is:

```c
struct Dog {
        Animal base;
};
```

Because `base` is still the first member, the address values of `Dog*` and `&dog->base` are the same in the current version.

However, `destroyDog()` is still written with the same pattern:

```c
static void destroyDog(Animal** self) {
        Dog* dog = container_of(*self, Dog, base);
        printf("I am %s. (destroy a dog)\n", (*self)->name);
        free(dog);
        *self = NULL;
}
```

The reason is not that `Dog` must be written this way right now. The purpose is to make the destruction rule independent of the current field order:

- Today, `Dog` has no private fields.
- Tomorrow, it may become `struct Dog { int age; Animal base; };`.
- As long as the complete object is recovered with `container_of(*self, Dog, base)`, the destruction path does not need to change its basic idea.

In other words, 003 is not only trying to show that “the code can run.” It is trying to show that “object destruction rules can be decoupled from object layout.”

## The Caller: After Entering the Abstract World, Both Calls and Destruction Use Only `Animal`

The structure of `main.c` clearly reflects the main point of this chapter:

```c
Cat* cat = newCat("Tom");
Dog* dog = newDog("Max");

Animal* animals[2] = {
        catAsAnimal(cat),
        dogAsAnimal(dog)
};
```

When creating objects, the caller still needs to know whether it is creating a `Cat` or a `Dog`. That is expected, because “which concrete type to create” is a concrete decision.

But once the objects are placed behind the abstract `Animal*` view:

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

The following behavior calls and lifetime termination no longer need to distinguish between concrete types.

This is exactly the missing part that 003 fills in:

```text
The “use” of an object can be abstracted.
The “end of lifetime” of an object can also be abstracted.
```

## What This Chapter Solves, and What It Does Not Solve Yet

What 003 already solves:

- When the abstraction layer only holds an `Animal*`, destruction can still be dispatched to the correct concrete type.
- The concrete type can release the correct complete object address, instead of incorrectly doing `free(base)`.
- The caller can end object lifetime through a unified interface.
- The `Animal*` variable passed in by the caller is set to `NULL` after destruction.

But the parts deliberately not expanded in this chapter are also important:

- For now, we only release the “object itself.”
- If a concrete type owns extra heap-allocated resources internally, this chapter has not yet formed a complete release chain.
- In other words, the current `destroyCat()` / `destroyDog()` is more like a simple final object release, not a full layered destructor system.

For example, imagine that `Cat` later becomes:

```c
struct Cat {
        char* favoriteFood;
        int lives;
        Animal base;
};
```

If `favoriteFood` is allocated dynamically through `malloc()`, `strdup()`, or a similar function, the destructor cannot simply call `free(cat)`. Otherwise, the object itself is released, but the heap memory pointed to by `favoriteFood` has not been handled.

So this chapter only discusses “simple destruction of the object itself.” It does not rush into all destructor layers at once.

## Summary

001 gave us polymorphic calls. 002 allowed us to recover the complete object from an `Animal*`. 003 combines these two abilities and fills in the lifetime-management part that had not been handled before.

From the interface perspective, `AnimalVtbl` no longer only describes “what this object can do.” It also begins to describe “how this object should end itself.”

From the implementation perspective, the key point of 003 is not `free()` itself, but the following sequence:

- First, use `container_of()` to recover the correct complete object address.
- Then, let the concrete type perform the release.
- Finally, clear the abstract pointer currently held by the caller.

This is already enough to form a clear, simple, and runnable destruction model.

## Preview of the Next Chapter

004 will continue from this chapter and extend “simple destruction” into a “destructor chain.”

At that point, the focus will no longer be only:

```c
free(the complete object);
```

Instead, it will become:

- First release the resources that the derived type itself allocated through `malloc()`, `strdup()`, or similar functions.
- Then connect the destruction actions layer by layer according to a convention.
- Finally release the outermost object itself.

In other words, 003 answers “who should release this object,” while 004 will continue to answer “in what order should the extra resources inside the object be released together?”

# From a First-Member Base Class to `container_of`: Taking Encapsulation in C One Step Further

- Author: Quirkybrain
- GitHub Repository: [Quirkybrain/C-learning-note](https://github.com/Quirkybrain/C-learning-note)

The example in `001-c-polymorphism-with-vtable` already showed how C can simulate encapsulation, abstraction, and polymorphism through structures and function-pointer tables. This time, I made one important change to `cat.c`: `Animal base` is no longer the first member of `struct Cat`. Instead, it is placed after `lives`.

This change looks small, but it pushes the example from “it happens to work” toward a style that is closer to real-world low-level engineering. Object conversion no longer depends on the hidden assumption that “the base-class field must be the first member of the structure.” Instead, we use a macro to recover the outer object address from the address of one of its members.

## Build and Run

This example still uses the `include/` and `src/` directory structure. Since the `container_of()` version relies on GNU C extensions such as `typeof` and statement expressions, the Makefile explicitly uses `-std=gnu11`.

```bash
make
make run
```

## What Changed

In `001-c-polymorphism-with-vtable`, the `Cat` structure looked like this:

```c
struct Cat {
        Animal base;
};
```

Then we added a new field, `lives`, after `base`:

```c
struct Cat {
        Animal base;
        int lives;
};
```

Because `base` is the first member, `Cat*` and `&cat->base` have the same address value. Therefore, inside `catSpeak()`, we could write this directly:

```c
static void catSpeak(Animal* self) {
        Cat* cat = (Cat*) self;
        printf("miaow~ I am %s, a cat, with %d lives.\n", self->name, cat->lives);
}
```

After compiling and running, we get:

```text
I am Tom. (Init a cat)
I am Max. (Init a dog)
miaow~ Tom drink water.
woof~ Max drink water.
miaow~ I am Tom, a cat, with 9 lives.
woof~ I am Max, a dog.
```

However, this depends on a very strong condition: `Animal base` must always be the first field of `struct Cat`. Once we move `lives` before `base`:

```c
struct Cat {
        int lives;
        Animal base;
};
```

`Cat*` points to the beginning of the whole object, while `Animal* self` points to the internal `base` member inside the object. Their addresses are no longer the same. If we still force-cast `Animal*` to `Cat*`, the result will not be the real starting address of the `Cat` object. Accessing `cat->lives` afterward is undefined behavior. It may read an incorrect value, or it may produce other unpredictable results:

```text
I am Tom. (Init a cat)
I am Max. (Init a dog)
miaow~ Tom drink water.
woof~ Max drink water.
miaow~ I am Tom, a cat, with 7171924 lives.
woof~ I am Max, a dog.
```

The key to solving this problem is:

```c
/**
 * Use container_of() to recover the outer object from a member pointer.
 */
Cat* cat = container_of(self, Cat, base);
```

`self` is still the `Animal*` passed in through the abstract interface, but `container_of()` uses the offset of the `base` member inside `Cat` to calculate the real address of the outer `Cat` object.

## How the Macro Locates the Object

The macro is defined as follows:

```c
/**
 * This container_of implementation uses GNU C extensions.
 * typeof and ({ ... }) are not ISO C syntax.
 * They are commonly available in GCC/Clang,
 * but may not work in strict C11 mode or with MSVC.
 */
#define my_offsetof(type, member) ((size_t)&(((type*)0)->member))

#define container_of(ptr, type, member) ({                  \
    const typeof( ((type*)0)->member )* __mptr = (ptr);    \
    (type*)( (char*)__mptr - my_offsetof(type, member) ); \
})
```

First, look at `my_offsetof(type, member)`:

```c
/**
 * A hand-written my_offsetof for learning purposes.
 * In production code, prefer offsetof from <stddef.h>.
 */
((size_t)&(((type*)0)->member))
```

The idea is this: assume there is a pointer of type `type*`, and its address is `0`. Then the address of `((type*)0)->member` is numerically equal to the offset of that member from the beginning of the structure.

For example:

```c
struct Cat {
        int lives;
        Animal base;
};
```

If the compiler decides that `base` is located 8 bytes after the beginning of the `Cat` object, then:

```c
my_offsetof(Cat, base)
```

will produce `8`. Here, `8` is only an example. The real offset should always be computed by the compiler, because structure alignment and padding are compiler-dependent.

Now look at `container_of(ptr, type, member)`:

```c
const typeof( ((type*)0)->member )* __mptr = (ptr);
```

This line first stores the incoming member pointer in `__mptr`. `typeof(((type*)0)->member)` extracts the type of `member`. In this example, `member` is `base`, so its type is `Animal`, and the type of `__mptr` becomes `const Animal*`.

This design has two benefits:

- `ptr` is evaluated only once, which avoids repeated execution when a macro argument has side effects.
- The compiler can perform a basic type check. If the pointer you pass in does not match the type of `member`, the problem is more likely to be exposed at compile time.

The final line performs the actual address recovery:

```c
(type*)( (char*)__mptr - my_offsetof(type, member) )
```

Here, we must first convert the pointer to `char*`, because pointer arithmetic in C moves according to the size of the pointed-to type. The size of `char` is 1 byte, so subtracting from a `char*` subtracts the exact number of bytes.

The full process can be understood like this:

```text
Start address of the Cat object
    |
    |  offset: offsetof(Cat, base)
    v
Address of the Animal base member, which is self
```

Therefore, to recover the original object, we simply reverse the calculation:

```text
self - offsetof(Cat, base) = start address of the Cat object
```

This is the essence of `container_of(self, Cat, base)`.

## Why This Encapsulation Style Is Useful

First, the memory layout of a concrete type is no longer locked by the abstraction layer.

In `001-c-polymorphism-with-vtable`, `Animal base` had to be the first field for `(Cat*)self` to work. This actually exposed the internal layout of `Cat` to the polymorphism mechanism. In this version, as long as `catAsAnimal()` returns `&cat->base`, and `catSpeak()` uses `container_of()` to recover the complete object, `base` can be the first member, the second member, or even placed later in the structure.

Second, concrete types can more naturally hold their own private state.

Now `Cat` has:

```c
int lives;
```

And `catSpeak()` can recover `Cat*` after receiving only an abstract `Animal*`:

```c
Cat* cat = container_of(self, Cat, base);
printf("miaow~ I am %s, a cat, with %d lives.\n", self->name, cat->lives);
```

The caller still only knows about `Animal*`, while the concrete implementation can still access its own `Cat`-specific data. This is an important part of making encapsulation and polymorphism work together: the public interface remains abstract, while the concrete implementation keeps its own state and behavior.

Third, the public interface becomes more stable.

`cat.h` only exposes an opaque type:

```c
typedef struct Cat Cat;
```

External code does not know how many fields `struct Cat` contains, nor does it know where `base` is located. In the future, we can add `lives`, `color`, `age`, or rearrange the field order inside `Cat`, and the caller does not need to change. This is the value of limiting change to the `.c` implementation file.

Fourth, this style is closer to real patterns used in low-level engineering.

The Linux kernel uses techniques similar to `container_of` extensively. A typical scenario is that a generic module only has a pointer to a common member inside a structure, such as a linked-list node, a reference-count object, or a work-queue node. From that member pointer, the code then recovers the real outer object that owns it. This chapter introduces that exact idea through `cat.c`.

## Complete `cat.c`

```c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cat.h"


/**
 * @brief A teaching version of an offset macro.
 *
 * In production code, prefer the standard offsetof(type, member) macro.
 * This hand-written version is used here to explicitly show the idea of
 * "the byte offset of a member from the beginning of a structure."
 */
#define my_offsetof(type, member) ((size_t)&(((type*)0)->member))

/**
 * @brief Recover the outer object pointer from a member pointer.
 *
 * ptr    : pointer to a member inside a structure.
 * type   : type of the outer structure.
 * member : name of the member pointed to by ptr.
 *
 * This version uses GNU C's typeof and statement-expression extensions,
 * so the Makefile explicitly enables gnu11 mode.
 */
#define container_of(ptr, type, member) ({                  \
        const typeof( ((type*)0)->member )* __mptr = (ptr);    \
        (type*)( (char*)__mptr - my_offsetof(type, member) ); \
})


/**
 * @brief The concrete Cat object.
 *
 * Here, the Cat-specific private field lives is intentionally placed before
 * Animal base. This demonstrates that once the base object is no longer at
 * the beginning of the structure, Animal* must not be directly cast to Cat*.
 * Instead, container_of() must be used to recover the complete object from
 * the member offset.
 */
struct Cat {
        /** @brief Cat's private state, proving that the object can own extra fields. */
        int lives;

        /** @brief The common base part exposed to the abstraction layer. */
        Animal base;
};


/**
 * @brief Initialize a newly allocated Cat object.
 *
 * @param self The Cat object to initialize.
 * @param name The name to copy into the embedded Animal base object.
 */
static void catInit(Cat* self, const char* name) {
        strncpy(self->base.name, name, MAX_NAME_LEN - 1);
        self->base.name[MAX_NAME_LEN-1] = 0;
        self->lives = 9;
        printf("I am %s. (Init a cat)\n", self->base.name);
}


/**
 * @brief Cat's implementation of AnimalVtbl::speak.
 *
 * The abstraction layer callback only passes in Animal*. Since base is not
 * the first member, self is not equal to the starting address of the Cat
 * object. Therefore, container_of() must be used here to recover the real
 * Cat* before safely accessing lives.
 *
 * @param self The Animal base pointer that belongs to a Cat object.
 */
static void catSpeak(Animal* self) {
        /* A direct cast would produce the wrong address; recover the full object using the base offset. */
        Cat* cat = container_of(self, Cat, base);
        printf("miaow~ I am %s, a cat, with %d lives.\n", self->name, cat->lives);
}


/**
 * @brief Cat's implementation of AnimalVtbl::drink.
 *
 * @param self The Animal base pointer that belongs to a Cat object.
 */
static void catDrink(Animal* self) {
        printf("miaow~ %s drink water.\n", self->name);
}

/**
 * @brief Bind the abstract speak/drink operations to Cat's concrete implementations.
 */
static const AnimalVtbl catVtbl = {
        .speak = catSpeak,
        .drink = catDrink
};

/**
 * @brief Allocate and initialize a Cat object.
 *
 * @param name The name to copy into the embedded Animal base object.
 * @return Pointer to the newly allocated Cat object, or NULL if allocation fails.
 */
Cat* newCat(const char* name) {
        Cat* cat = (Cat*) malloc (sizeof(Cat));
        if (cat == NULL) return NULL;

        /* Polymorphic dispatch depends on this table, so bind it immediately after object creation. */
        cat->base.vtblptr = &catVtbl;
        catInit(cat, name);

        return cat;
}

/**
 * @brief Release a Cat object created by newCat().
 *
 * @param cat The Cat object to release.
 */
void deleteCat(Cat* cat) {
        free(cat);
}

/**
 * @brief View Cat as its embedded Animal base object.
 *
 * This returns the address of the internal base member, not the starting
 * address of the Cat object itself. The caller only receives the abstract
 * view, while the concrete implementation can later use container_of() to
 * recover the complete object.
 *
 * @param cat The Cat object to convert.
 * @return Pointer to the embedded Animal base object.
 */
Animal* catAsAnimal(Cat* cat) {
        return &(cat->base);
}
```

## Summary

This chapter shows one of the most interesting aspects of C. The language does not have a built-in class system, but it is very close to the memory model. As long as we understand addresses, offsets, function pointers, and structure layout, we can build readable and maintainable abstractions on top of a very thin mechanism.

More importantly, we now have a crucial ability for the next step: when the abstraction layer only gives us an `Animal*`, the concrete type can still recover the complete object address.

This prepares the ground for the next chapter. Since we can now safely recover the real `Cat*` or `Dog*`, the next step is to add a “destructor” into the function table as well. Then object destruction no longer needs to be written separately by the caller as `deleteCat()` and `deleteDog()`. Instead, it can go through the same unified abstract interface as `speak` and `drink`.

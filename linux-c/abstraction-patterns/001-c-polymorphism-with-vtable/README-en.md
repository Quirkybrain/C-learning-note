# Implementing Encapsulation, Abstraction, and Polymorphism in C with Structs and Function Tables

- Author: Quirkybrain
- GitHub Repository: [Quirkybrain/C-learning-note](https://github.com/Quirkybrain/C-learning-note)

C does not provide language features such as `class`, `virtual`, or `override`, nor does it have built-in object-oriented programming mechanisms.

However, this does not mean that C cannot express ideas such as **encapsulation**, **abstraction**, and **polymorphism**.

This example uses a simple `Animal` / `Cat` / `Dog` model to demonstrate a common C programming technique:

- Use structures to store object data.
- Use a function pointer table to describe a group of abstract behaviors.
- Let each concrete type provide its own function table inside its implementation file.
- Let the caller invoke behavior only through abstract interfaces, while the function table stored inside the runtime object determines which concrete function is actually executed.

The final result is that the same calls, such as `animalDrink()` and `animalSpeak()`, behave differently depending on the object passed in. When the object is a cat, cat-specific behavior is executed; when the object is a dog, dog-specific behavior is executed.

## Building and Running

The current project is organized with `include/` and `src/` directories. You can build and run it directly inside the project directory:

```bash
make
make run
```

## Project Structure

This example consists of the following files:

```text
.
├── include
│   ├── animal.h   # Abstract layer: Animal base type and function table definition
│   ├── cat.h      # Public interface for Cat
│   └── dog.h      # Public interface for Dog
├── src
│   ├── animal.c   # Dispatch implementation for the abstract interface
│   ├── cat.c      # Private Cat structure, function table, and concrete behavior
│   └── dog.c      # Private Dog structure, function table, and concrete behavior
├── main.c         # Caller-side example
└── Makefile       # Build script
```

The most important abstraction relationship is:

```text
Cat / Dog object
    ↓ embeds
Animal base
    ↓ holds
AnimalVtbl* vtblptr
    ↓ points to
Concrete type's own function table: catVtbl / dogVtbl
    ↓ dispatches to
catSpeak / dogSpeak
catDrink / dogDrink
```

## The Abstract Layer: `Animal` and the Function Table

`animal.h` is the core of the whole design. It defines two key structures:

- `AnimalVtbl`: a function table that describes what behaviors an animal should support.
- `Animal`: a common base object that stores the name and a pointer to the function table.

Here, `AnimalVtbl` is similar to the virtual function table behind a C++ object. The difference is that C does not automatically generate or maintain it. Everything must be written manually.

### `animal.h`

```c
/**
 * @file animal.h
 * @author quirkybrain
 * @brief Abstract base interface used to simulate polymorphism with structs and function pointer tables.
 * @version 0.1
 * @date 2026-05-20
 */
#ifndef ANIMAL_H_
#define ANIMAL_H_


/** @brief Maximum number of bytes available for an animal name, including the trailing '\0'. */
#define MAX_NAME_LEN 24


/** @brief Forward declaration of the abstract Animal base type. */
typedef struct Animal Animal;

/** @brief Function table type describing the operations supported by an Animal object. */
typedef struct AnimalVtbl AnimalVtbl;

/**
 * @brief Behavior table that concrete animal types need to provide.
 *
 * The caller always interacts through Animal* and this table. It does not
 * directly depend on the concrete Cat/Dog implementation functions, so this
 * table plays the role of an abstract interface in this example.
 */
struct AnimalVtbl {
        /** @brief Make the animal speak. self points to the embedded Animal base object. */
        void (*speak)(Animal* self);

        /** @brief Make the animal drink. self points to the embedded Animal base object. */
        void (*drink)(Animal* self);
};

/**
 * @brief Common base object embedded inside every concrete animal type.
 *
 * Cat and Dog both embed this structure as one of their members.
 * The abstract layer only knows Animal, so this structure stores the minimum
 * common data shared across concrete types: the name and a pointer to the
 * concrete behavior table.
 */
struct Animal {
        /** @brief Display name of the animal. */
        char name[MAX_NAME_LEN];

        /** @brief Pointer to the concrete type's function table for runtime dispatch. */
        const AnimalVtbl* vtblptr;
};

/**
 * @brief Dispatch the speak operation through the object's function table.
 *
 * @param self Animal base pointer, usually obtained from the base member of a concrete object.
 */
void animalSpeak(Animal* self);

/**
 * @brief Dispatch the drink operation through the object's function table.
 *
 * @param self Animal base pointer, usually obtained from the base member of a concrete object.
 */
void animalDrink(Animal* self);

#endif
```

The key abstraction is that `animalSpeak()` and `animalDrink()` do not care whether the object passed in is actually a `Cat` or a `Dog`.

They only require an `Animal*`, and then use `self->vtblptr` to find the real behavior implementation.

### `animal.c`

```c
#include "animal.h"

/**
 * @brief Dispatch the drink behavior using the function table currently bound to the object.
 *
 * This layer does not contain knowledge about how a cat drinks or how a dog
 * drinks. It only reads the vtblptr inside the object and forwards the call
 * to the corresponding implementation.
 *
 * @param self Pointer to the Animal base embedded inside a concrete object.
 */
void animalDrink(Animal* self) {
        self->vtblptr->drink(self);
}

/**
 * @brief Dispatch the speak behavior using the function table currently bound to the object.
 *
 * @param self Pointer to the Animal base embedded inside a concrete object.
 */
void animalSpeak(Animal* self) {
        self->vtblptr->speak(self);
}
```

These two functions are the entry points for polymorphic dispatch:

```c
self->vtblptr->drink(self);
self->vtblptr->speak(self);
```

They do not directly call `catDrink()`, `dogDrink()`, `catSpeak()`, or `dogSpeak()`. Instead, they call the corresponding function indirectly through the function table stored inside the object.

This allows the same interface to show different behavior depending on the actual object type.

## Concrete Type: `Cat`

The public header file for `Cat` only exposes an opaque type and a small set of operations. The caller knows that a `Cat` type exists, but it does not know what fields are inside `struct Cat`.

This is a common way to implement encapsulation in C: declare the type in the `.h` file, and define the structure details in the `.c` file.

### `cat.h`

```c
/**
 * @file cat.h
 * @author quirkybrain
 * @brief Public interface for the concrete Cat type.
 * @version 0.1
 * @date 2026-05-20
 */
#ifndef CAT_H_
#define CAT_H_


#include "animal.h"


/** @brief Opaque concrete Cat type. The caller can only operate on it through public functions. */
typedef struct Cat Cat;

/**
 * @brief Allocate and initialize a Cat object.
 *
 * @param name Name to be copied into the embedded Animal base object.
 * @return Pointer to the newly created Cat object; returns NULL if memory allocation fails.
 */
Cat* newCat(const char* name);

/**
 * @brief Release a Cat object created by newCat().
 *
 * @param cat Cat object to release.
 */
void deleteCat(Cat* cat);

/**
 * @brief Treat a Cat object as its embedded Animal base object.
 *
 * @param cat Cat object to convert.
 * @return Pointer to the embedded Animal base object. The returned value is valid only while cat is alive.
 */
Animal* catAsAnimal(Cat* cat);

#endif
```

Pay attention to this line:

```c
typedef struct Cat Cat;
```

The fields of `struct Cat` are not provided here. Therefore, to external callers, `Cat` is an opaque type.

The caller cannot directly access the internal data of a cat object. It can only operate on the object through public functions such as `newCat()`, `deleteCat()`, and `catAsAnimal()`.

### `cat.c`

```c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cat.h"


/**
 * @brief Concrete Cat object.
 *
 * In this example, Animal base is placed as the first member so that the Cat
 * object and its embedded Animal subobject have the same starting address.
 * This allows Cat* to be naturally converted upward to Animal* for polymorphic dispatch.
 */
struct Cat {
        /** @brief Common base part exposed to the abstract layer. */
        Animal base;
};


/**
 * @brief Initialize a newly allocated Cat object.
 *
 * This writes the name into the common base area. Since this is a teaching
 * example, only the minimal initialization logic is kept.
 *
 * @param self Cat object to initialize.
 * @param name Name to copy into the Animal base.
 */
static void catInit(Cat* self, const char* name) {
        strncpy(self->base.name, name, MAX_NAME_LEN - 1);
        self->base.name[MAX_NAME_LEN-1] = 0;
        printf("I am %s. (Init a cat)\n", self->base.name);
}

/**
 * @brief Cat implementation of AnimalVtbl::speak.
 *
 * This callback only accesses the common fields already available in the
 * Animal base, so it does not need to cast self back to Cat*.
 *
 * @param self Animal base pointer that belongs to a Cat object.
 */
static void catSpeak(Animal* self) {
        printf("miaow~ I am %s, a cat.\n", self->name);
}

/**
 * @brief Cat implementation of AnimalVtbl::drink.
 *
 * @param self Animal base pointer that belongs to a Cat object.
 */
static void catDrink(Animal* self) {
        printf("miaow~ %s drink water.\n", self->name);
}

/**
 * @brief Bind the abstract speak/drink operations to Cat-specific implementations.
 *
 * Every Cat object stores a pointer to this static table in base.vtblptr.
 */
static const AnimalVtbl catVtbl = {
        .speak = catSpeak,
        .drink = catDrink
};

/**
 * @brief Allocate and initialize a Cat object.
 *
 * @param name Name to copy into the embedded Animal base object.
 * @return Pointer to the newly created Cat object; returns NULL if allocation fails.
 */
Cat* newCat(const char* name) {
        Cat* cat = (Cat*) malloc (sizeof(Cat));
        if (cat == NULL) return NULL;

        /* Bind the behavior table first, then initialize common data. */
        cat->base.vtblptr = &catVtbl;
        catInit(cat, name);

        return cat;
}

/**
 * @brief Release a Cat object created by newCat().
 *
 * @param cat Cat object to release.
 */
void deleteCat(Cat* cat) {
        free(cat);
}

/**
 * @brief Treat Cat as its embedded Animal base object.
 *
 * Because base is the first member, the returned address has the same numeric
 * value as the starting address of cat. However, the exposed type is still
 * the more abstract Animal*.
 *
 * @param cat Cat object to convert.
 * @return Pointer to the embedded Animal base object.
 */
Animal* catAsAnimal(Cat* cat) {
        return &(cat->base);
}
```

There are three key design points in `Cat`.

First, `Cat` embeds an `Animal base`:

```c
struct Cat {
        Animal base;
};
```

This means that a cat object “has an Animal base part.” Common fields and the function table pointer are stored inside this `base`.

Second, `catSpeak()` and `catDrink()` are `static` functions:

```c
static void catSpeak(Animal* self)
static void catDrink(Animal* self)
```

The `static` keyword makes them visible only inside `cat.c`. External code cannot call them directly; it can only reach them indirectly through `AnimalVtbl`.

This is also a form of encapsulation.

Third, `catVtbl` binds the abstract behavior to concrete implementations:

```c
static const AnimalVtbl catVtbl = {
        .speak = catSpeak,
        .drink = catDrink
};
```

When `newCat()` creates an object, it writes Cat's own function table into `base.vtblptr`:

```c
cat->base.vtblptr = &catVtbl;
```

From that moment on, when the object is used as an `Animal*`, calling `animalSpeak()` dispatches to `catSpeak()`, and calling `animalDrink()` dispatches to `catDrink()`.

## Concrete Type: `Dog`

The design of `Dog` is almost the same as `Cat`. The difference is that it binds dog-specific behavior functions.

This repeated structure shows the value of the abstraction layer: as long as a concrete type follows the contract defined by `AnimalVtbl`, it can be used uniformly as an `Animal`.

### `dog.h`

```c
/**
 * @file dog.h
 * @author quirkybrain
 * @brief Public interface for the concrete Dog type.
 * @version 0.1
 * @date 2026-05-20
 */
#ifndef DOG_H_
#define DOG_H_


#include "animal.h"


/** @brief Opaque concrete Dog type. The caller can only operate on it through public functions. */
typedef struct Dog Dog;

/**
 * @brief Allocate and initialize a Dog object.
 *
 * @param name Name to be copied into the embedded Animal base object.
 * @return Pointer to the newly created Dog object; returns NULL if allocation fails.
 */
Dog* newDog(const char* name);

/**
 * @brief Release a Dog object created by newDog().
 *
 * @param dog Dog object to release.
 */
void deleteDog(Dog* dog);

/**
 * @brief Treat a Dog object as its embedded Animal base object.
 *
 * @param dog Dog object to convert.
 * @return Pointer to the embedded Animal base object. The returned value is valid only while dog is alive.
 */
Animal* dogAsAnimal(Dog* dog);

#endif
```

### `dog.c`

```c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "dog.h"


/**
 * @brief Concrete Dog object.
 *
 * Like Cat, Dog also places Animal base as the first member.
 * This ensures that the base pointer returned by dogAsAnimal() can directly
 * participate in unified dispatch.
 */
struct Dog {
        /** @brief Common base part exposed to the abstract layer. */
        Animal base;
};


/**
 * @brief Initialize a newly allocated Dog object.
 *
 * @param self Dog object to initialize.
 * @param name Name to copy into the Animal base.
 */
static void dogInit(Dog* self, const char* name) {
        strncpy(self->base.name, name, MAX_NAME_LEN - 1);
        self->base.name[MAX_NAME_LEN-1] = 0;
        printf("I am %s. (Init a dog)\n", self->base.name);
}

/**
 * @brief Dog implementation of AnimalVtbl::speak.
 *
 * This callback only depends on the common base field name, so it does not
 * need to access Dog-specific private state.
 *
 * @param self Animal base pointer that belongs to a Dog object.
 */
static void dogSpeak(Animal* self) {
        printf("woof~ I am %s, a dog.\n", self->name);
}

/**
 * @brief Dog implementation of AnimalVtbl::drink.
 *
 * @param self Animal base pointer that belongs to a Dog object.
 */
static void dogDrink(Animal* self) {
        printf("woof~ %s drink water.\n", self->name);
}

/**
 * @brief Bind the abstract speak/drink operations to Dog-specific implementations.
 */
static const AnimalVtbl dogVtbl = {
        .speak = dogSpeak,
        .drink = dogDrink
};

/**
 * @brief Allocate and initialize a Dog object.
 *
 * @param name Name to copy into the embedded Animal base object.
 * @return Pointer to the newly created Dog object; returns NULL if allocation fails.
 */
Dog* newDog(const char* name) {
        Dog* dog = (Dog*) malloc (sizeof(Dog));
        if (dog == NULL) return NULL;

        /* Bind the function table first, then write common state such as the name. */
        dog->base.vtblptr = &dogVtbl;
        dogInit(dog, name);

        return dog;
}

/**
 * @brief Release a Dog object created by newDog().
 *
 * @param dog Dog object to release.
 */
void deleteDog(Dog* dog) {
        free(dog);
}

/**
 * @brief Treat Dog as its embedded Animal base object.
 *
 * Since base is located at the beginning of the structure, this operation is
 * essentially a manual upcast.
 *
 * @param dog Dog object to convert.
 * @return Pointer to the embedded Animal base object.
 */
Animal* dogAsAnimal(Dog* dog) {
        return &(dog->base);
}
```

There is also one very important line in the Dog constructor:

```c
dog->base.vtblptr = &dogVtbl;
```

This means that every concrete object must attach its own function table to the embedded `Animal base` during initialization. Otherwise, the abstract layer would not know which concrete function to dispatch to.

## Caller Side: Depending Only on the Abstract Interface

In `main.c`, objects are still created through concrete constructors:

```c
Cat* cat = newCat("Tom");
Dog* dog = newDog("Max");
```

But when behavior is invoked, the program goes through the abstract interface:

```c
animalDrink(catAsAnimal(cat));
animalDrink(dogAsAnimal(dog));

animalSpeak(catAsAnimal(cat));
animalSpeak(dogAsAnimal(dog));
```

The caller invokes the same group of functions: `animalDrink()` and `animalSpeak()`.

The different output is not determined by `if` or `switch` statements in `main.c`. Instead, it is determined by the `vtblptr` stored inside each object.

### `main.c`

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

Here, `catAsAnimal()` and `dogAsAnimal()` can be understood as manual “upcasting”:

```c
Animal* catAsAnimal(Cat* cat) {
        return &(cat->base);
}
```

It extracts the `Animal base` from the concrete object, allowing the caller to process different concrete types through a unified `Animal*`.

## Compilation and Output

For convenience, this example uses a simple `Makefile`.

### `Makefile`

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

Run:

```sh
make run
```

Example output:

```text
I am Tom. (Init a cat)
I am Max. (Init a dog)
miaow~ Tom drink water.
woof~ Max drink water.
miaow~ I am Tom, a cat.
woof~ I am Max, a dog.
```

From the output, we can see that the same abstract calls:

```c
animalDrink(...)
animalSpeak(...)
```

produce `miaow~` behavior for a `Cat` object and `woof~` behavior for a `Dog` object.

This is the polymorphic effect implemented through a function pointer table.

## Several Important Design Points

### 1. Encapsulation with Opaque Types

In `cat.h` and `dog.h`, we only write:

```c
typedef struct Cat Cat;
typedef struct Dog Dog;
```

The real structure definitions are placed in `cat.c` and `dog.c`:

```c
struct Cat {
        Animal base;
};
```

This prevents the caller from directly accessing the internal fields of `Cat` or `Dog`. The caller can only create, destroy, or convert objects through public functions.

This is a common encapsulation technique in C: header files expose interfaces, while source files hide implementation details.

### 2. Expressing an Abstract Interface with a Function Table

`AnimalVtbl` defines a group of behaviors that an animal should support:

```c
struct AnimalVtbl {
        void (*speak)(Animal* self);
        void (*drink)(Animal* self);
};
```

It does not care how a specific animal speaks or drinks. It only specifies that each concrete type must provide two functions: `speak` and `drink`.

Therefore, `AnimalVtbl` acts as the abstract interface in this example.

### 3. Simulating Inheritance with an Embedded Base Object

Each concrete type embeds `Animal` inside its own structure:

```c
struct Cat {
        Animal base;
};

struct Dog {
        Animal base;
};
```

This gives both `Cat` and `Dog` a shared piece of `Animal` data, including `name` and `vtblptr`.

This is not inheritance at the C language level. It is a composition-based engineering convention: a concrete object contains a common base object, and external code operates on that common base object through a unified interface.

### 4. Runtime Dispatch with `vtblptr`

During initialization, each object stores its own function table:

```c
cat->base.vtblptr = &catVtbl;
dog->base.vtblptr = &dogVtbl;
```

The abstract interface then dispatches through that table:

```c
self->vtblptr->drink(self);
self->vtblptr->speak(self);
```

Therefore, `animalDrink()` does not need to know whether `self` comes from a cat or a dog. It only needs to trust that `self->vtblptr` already points to the correct function table.

### 5. New Types Can Be Added Using the Same Pattern

If we want to add a `Bird` type later, the process is roughly:

1. Define the public interface, such as `bird.h`.
2. Define `struct Bird { Animal base; };` in `bird.c`.
3. Implement `birdSpeak()` and `birdDrink()`.
4. Define `birdVtbl`.
5. Set `bird->base.vtblptr = &birdVtbl;` in `newBird()`.
6. Provide `birdAsAnimal()` to return `&(bird->base)`.

After that, the caller can still use it through `animalSpeak()` and `animalDrink()`.

## Summary

This example demonstrates a simple but very important abstraction technique in C:

- `struct` organizes object data.
- The boundary between `.h` and `.c` hides implementation details.
- Function pointers describe replaceable behavior.
- Function tables package a group of behaviors into an interface.
- The function table pointer inside an object selects the concrete implementation at runtime.

In one sentence:

> In C, we can manually implement concepts similar to encapsulation, abstraction, and polymorphism by combining an embedded common base structure, a function pointer table, and public dispatch functions.

This technique does not turn C into C++. It does not automatically provide type checking, inheritance hierarchy management, or destructor chains.

Instead, it is a clear engineering convention: as long as each object correctly initializes its function table, the caller can process different concrete types through a unified interface.

## Preview of the Next Chapter

In this chapter, both `Cat` and `Dog` place `Animal base` as the first member of the structure. Therefore, the numeric address of `Cat*` / `Dog*` and `&obj->base` happens to be the same, which makes many conversions look natural.

However, in real projects, derived types often add their own private fields. Once `Animal base` is no longer located at the beginning of the structure, the `Animal*` passed into the abstract interface is no longer equal to the starting address of the complete object. Directly casting it back to `Cat*` or `Dog*` would then be incorrect.

The next chapter will continue from this problem:

> When the base member is no longer the first member of a derived structure, how can we recover the address of the outer object from the address of one of its members?

This is exactly why techniques such as `container_of` work and why they are so important in real C projects.

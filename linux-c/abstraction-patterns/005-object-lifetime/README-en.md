# From Direct Destruction to Reference Counting: Shared Object Lifetimes in C

- Author: Quirkybrain
- GitHub repository: [Quirkybrain/C-learning-note](https://github.com/Quirkybrain/C-learning-note)

Over the previous chapters, I gradually built a simple object model in C: function tables organize common behavior, `container_of()` recovers the complete object, and `cleanUp -> release` cleans up member resources before releasing the object itself.

004 explained how an object should be destroyed, but the caller used `destroyAnimal()`:

```text
Destroy this object now.
```

When several owners share the same object, one owner no longer using it does not mean that every other owner has also left. This chapter therefore adds a reference count to `Animal` and uses a `get/put` protocol to manage the shared lifetime:

```text
get: add an owner
put: the current owner leaves
reference count reaches zero: actually destroy the object
```

## Build and Run

```bash
make
make run
```

Output:

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

The first `animalPut()` only releases one reference, so the object remains alive. The second `animalPut()` reduces the reference count to zero and triggers final destruction.

## The Problem with Direct Destruction in 004

In 004, the caller could directly end an object's lifetime:

```c
Animal* animal = dogAsAnimal(dog);
Animal* sharedAnimal = animal;

destroyAnimal(&animal);
```

`animal` and `sharedAnimal` originally point to the same Dog. After `destroyAnimal()` is called, the Dog's resources have been released, but the other owner still holds the original address.

Direct destruction can only express “release the object immediately.” It cannot express “the current owner is leaving while another owner is still using the object.” Therefore, in 005, no single owner directly decides when destruction happens. Instead, the object tracks how many owning references remain.

## Adding a Reference Count to Animal

005 adds `refCount` to the common `Animal` object:

```c
struct Animal {
        char name[MAX_NAME_LEN];
        const AnimalVtbl* vtblptr;
        unsigned int refCount;
};
```

The shared-lifetime rule applies to every concrete type, so the reference count belongs in the common object. As long as the abstraction layer has an `Animal*`, it can increase or release references through one unified interface.

When object creation finishes, the caller already holds the first reference. Both Cat and Dog therefore initialize the count to 1:

```c
self->base.refCount = 1;
```

```text
object        0 -> 1
```

## animalGet: Acquire a New Reference

`animalGet()` adds another owner:

```c
Animal* animalGet(Animal* self) {
        assert(self->refCount > 0);
        ++self->refCount;

        return self;
}
```

Usage:

```c
Animal* sharedAnimal = animalGet(animal);
```

The returned pointer still refers to the same object, but the reference count changes from 1 to 2:

```text
animal         ──┐
                 ├──> Dog, refCount = 2
sharedAnimal   ──┘
```

Each call to `animalGet()` acquires a new owning reference and must eventually be matched by one call to `animalPut()`.

## animalPut: Release a Reference

`animalPut()` means that the current owner no longer uses the object:

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

It first saves the object address and clears the caller's pointer, then decreases the reference count:

- If the count is still greater than 0, another owner remains and the object stays alive.
- If the count becomes 0, `cleanUp` and `release` are called to end the object's lifetime.

The function still receives an `Animal**` because `animalPut()` needs to set the caller's pointer to `NULL`:

```c
animalPut(&animal);
```

## The Complete Lifetime in main

`main()` first creates a Dog and then uses it through `Animal*` for the rest of the example:

```c
Dog* dog = newDog("Max", "bone");

Animal* animal = dogAsAnimal(dog);
dog = NULL;
```

The object now has one initial reference. A second owner is then created:

```c
Animal* sharedAnimal = animalGet(animal);
```

Both owners can use the same object:

```c
animalSpeak(animal);
animalSpeak(sharedAnimal);
```

The two references are then released in order:

```c
animalPut(&animal);
animalSpeak(sharedAnimal);

animalPut(&sharedAnimal);
```

The complete reference-count sequence is:

| Operation | `refCount` | Result |
|---|---:|---|
| `newDog()` | 1 | Establish the initial reference |
| `animalGet(animal)` | 2 | `sharedAnimal` becomes the second owner |
| `animalPut(&animal)` | 1 | `animal` leaves; the object stays alive |
| `animalPut(&sharedAnimal)` | 0 | The last owner leaves; destroy the object |

The final `put` triggers the following process for Dog:

```text
animalPut(&sharedAnimal)
    -> refCount: 1 -> 0
    -> cleanUpDog()
        -> free(dog->foodName)
    -> releaseDog()
        -> free(dog)
```

This combines the responsibilities introduced in 004 and 005:

```text
Reference counting decides when to destroy.
The destruction chain decides how to destroy.
```

## Pointers and References Are Not the Same Thing

Directly copying a pointer only copies an address:

```c
Animal* sharedAnimal = animal;
```

It does not change `refCount`. Acquiring a new owning reference requires an explicit call:

```c
Animal* sharedAnimal = animalGet(animal);
```

The reference count tracks owners registered through the lifetime protocol, not the number of times the same address appears. The basic rule is:

```text
The initial reference from creation -> one put
Each get                           -> one put
```

This `get/put` naming pattern is common in C projects. Readers can also explore Linux's `kref`, `kobject_get()/kobject_put()`, and `get_device()/put_device()` interfaces.

## Summary

004 uses `cleanUp -> release` to define the final destruction order, while 005 uses reference counting to decide when the object enters that destruction chain:

```text
new
    -> refCount = 1
    -> get adds an owner
    -> put releases an owner
    -> refCount = 0
        -> cleanUp
        -> release
```

The core idea of this chapter is one sentence:

```text
Each owner holds one reference, each reference requires one put,
and the final put ends the object's lifetime.
```

## Next Chapter

006 will introduce intrusive linked lists: embedding a list node directly into an object and using `container_of()` to recover the complete object from that node, moving from one shared object to organizing and traversing a collection of objects.

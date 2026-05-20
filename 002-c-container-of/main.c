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
}

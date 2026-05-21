#include "cat.h"
#include "dog.h"

int main(void) {
        Cat* cat = newCat("Tom");
        Dog* dog = newDog("Max");

        Animal* animals[2] = {
                catAsAnimal(cat),
                dogAsAnimal(dog)
        };

        for (int i = 0; i < 2; ++i) {
                animalDrink(animals[i]);
        }

        for (int i = 0; i < 2; ++i) {
                animalSpeak(animals[i]);
        }

        for (int i = 0; i < 2; ++i) {
                destroyAnimal(&animals[i]);
        }

        return 0;
}

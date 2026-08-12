#include <stdio.h>
#include "cat.h"
#include "dog.h"


/**
 * @brief 演示 Dog 对象从创建、共享到最终释放的完整生命周期。
 *
 * main 只在构造阶段短暂持有 Dog*，交接完成后统一通过 Animal* 管理对象。
 * animal 和 sharedAnimal 分别代表一个 owner，用来展示“释放一个引用”和
 * “释放最后一个引用”对对象生命周期产生的不同影响。
 *
 * @return 程序正常结束时返回 0。
 */
int main(void) {
	printf("\n============================================================\n");
	printf("  [STEP 1] CREATE A DOG AND SHARE ITS OWNERSHIP\n");
	printf("============================================================\n");
	printf("[CREATE] Create Dog Max and expose it through Animal.\n\n");

	/*
         * 创建完整的具体 Dog 对象，同时产生第一个拥有引用。
         * 
         * dog object        0    ->    1
         * 
         */
	Dog* dog = newDog("Max", "bone");

	/* 取得内嵌 Animal 基类的地址，把初始引用交给通用层指针管理。 */
        Animal* animal = dogAsAnimal(dog);

	/* 交接完成后不再通过具体类型指针持有或访问这个对象。 */
        dog = NULL;
        
	printf("[GET] Create a second owner: sharedAnimal.\n");
	/* get 创建第二个 owner，此时两个 Animal* 共享同一个 Dog 对象。
         * 
         * dog object        1    ->    2
         * 
         */
        Animal* sharedAnimal = animalGet(animal);

        animalSpeak(animal);
        animalSpeak(sharedAnimal);

	printf("\n============================================================\n");
	printf("  [STEP 2] RELEASE OWNER: animal\n");
	printf("============================================================\n");
	printf("[PUT] Release animal, then verify that sharedAnimal keeps the object alive.\n\n");

	/* 只释放 animal 对应的引用并把 animal 置为 NULL。
         * 
         * dog object        2    ->    1
         * 
         */
        animalPut(&animal);

	if (animal == NULL) {
		printf("[OWNER RELEASED] animal is now NULL.\n");
	}

	if (sharedAnimal != NULL) {
		/* sharedAnimal 仍然持有引用，所以底层 Dog 对象仍可安全使用。 */
		animalSpeak(sharedAnimal);
		printf("[OBJECT ALIVE] sharedAnimal still owns the dog.\n");
	}

	printf("\n============================================================\n");
	printf("  [STEP 3] RELEASE THE FINAL OWNER: sharedAnimal\n");
	printf("============================================================\n");
	printf("[FINAL PUT] Releasing sharedAnimal will destroy the dog.\n\n");

	/* 最后一个引用归零，触发 cleanUpDog() 和 releaseDog()。
         * 
         * dog object        1    ->    0
         * 
         */
        animalPut(&sharedAnimal);

	/* 两个 owner 都已清空，此后不能再访问原来的 Dog 对象。 */
	if (animal == NULL && sharedAnimal == NULL) {
		printf("\n************************************************************\n");
		printf("  [OBJECT DESTROYED] No owners remain.\n");
		printf("************************************************************\n");
	} 

        return 0;
}

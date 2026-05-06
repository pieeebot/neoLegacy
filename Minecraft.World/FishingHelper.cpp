
#include "../Minecraft.World/WeighedTreasure.h"
#include "../Minecraft.World/Biome.h"
#include "../Minecraft.World/FishingHelper.h"
#include "../Minecraft.World/ItemInstance.h"
#include "../Minecraft.World/EnchantmentHelper.h"
#include "net.minecraft.world.item.h"
#include <memory>

FishingHelper* FishingHelper::getInstance()
{
	static FishingHelper instance;
	return &instance;
}

FishingHelper::FishingHelper() : fishingFishArray(4), fishingJunkArray(11), fishingTreasuresArray(6)
{
	fishingTreasuresArray[0] = new CatchWeighedItem(Item::bow_Id, 1, 0, 1);
	fishingTreasuresArray[1] = new CatchWeighedItem(Item::book_Id, 1, 0, 1);
	fishingTreasuresArray[2] = new CatchWeighedItem(Item::fishing_rod_Id, 1, 0, 1);
	fishingTreasuresArray[3] = new CatchWeighedItem(Item::name_tag_Id, 1, 0, 1);
	fishingTreasuresArray[4] = new CatchWeighedItem(Item::saddle_Id, 1, 0, 1);
	fishingTreasuresArray[5] = new CatchWeighedItem(Tile::waterlily_Id, 1, 0, 1);

	fishingFishArray[0] = new CatchWeighedItem(Item::fish_Id, 1, 0, 60); // Fish
	fishingFishArray[1] = new CatchWeighedItem(Item::fish_Id, 1, 1, 25); // Salmon
	fishingFishArray[2] = new CatchWeighedItem(Item::fish_Id, 1, 2, 2); // Clownfish
	fishingFishArray[3] = new CatchWeighedItem(Item::fish_Id, 1, 3, 13); // Pufferfish

	fishingJunkArray[0] = new CatchWeighedItem(Item::leather_Id, 1, 0, 10);
	fishingJunkArray[1] = new CatchWeighedItem(Item::bone_Id, 1, 0, 10);
	fishingJunkArray[2] = new CatchWeighedItem(Item::potion_Id, 1, 0, 10); // Water bottle
	fishingJunkArray[3] = new CatchWeighedItem(Item::bowl_Id, 1, 0, 10);
	fishingJunkArray[4] = new CatchWeighedItem(Item::leather_boots_Id, 1, 0, 10);
	fishingJunkArray[5] = new CatchWeighedItem(Item::rotten_flesh_Id, 1, 0, 10);
	fishingJunkArray[6] = new CatchWeighedItem(Tile::tripwire_hook_Id, 1, 0, 10);
	fishingJunkArray[7] = new CatchWeighedItem(Item::stick_Id, 1, 0, 5);
	fishingJunkArray[8] = new CatchWeighedItem(Item::string_Id, 1, 0, 5);
	fishingJunkArray[9] = new CatchWeighedItem(Item::fishing_rod_Id, 1, 0, 2);
	fishingJunkArray[10] = new CatchWeighedItem(Item::dye_Id, 10, 0, 1); // 10 ink sacs
}

CatchType FishingHelper::getRandCatchType(int luckLevel, int lureLevel, Random* random)
{
	float randFloat = random->nextFloat();
	float junkChance = 0.1F - (float)luckLevel * 0.025F - (float)lureLevel * 0.01F; // default 10% chance, affected by lure and luck of the sea
	float treasureChance = 0.05F + (float)luckLevel * 0.01F - (float)lureLevel * 0.01F; // default 5% chance, affected by lure and luck of the sea
	junkChance = clamp(junkChance, 0.0F, 1.0F);
	treasureChance = clamp(treasureChance, 0.0F, 1.0F);

	if (randFloat < junkChance)
	{
		return CatchType::JUNK;
	}
	else
	{
		randFloat -= junkChance;

		if (randFloat < treasureChance)
		{
			return CatchType::TREASURE;
		}
		else
		{
			return CatchType::FISH;
		}
	}
}

CatchWeighedItem* FishingHelper::getRandCatch(CatchType catchType, Random* random)
{
	switch (catchType) {
		case CatchType::FISH:
			return static_cast<CatchWeighedItem *>(WeighedRandom::getRandomItem(random, fishingFishArray));
		case CatchType::TREASURE:
			return static_cast<CatchWeighedItem *>(WeighedRandom::getRandomItem(random, fishingTreasuresArray));
		case CatchType::JUNK:
			return static_cast<CatchWeighedItem *>(WeighedRandom::getRandomItem(random, fishingJunkArray));
	}
}

std::shared_ptr<ItemInstance> FishingHelper::handleCatch(CatchWeighedItem* weighedCatch, CatchType catchType, Random* random)
{	
	std::shared_ptr<ItemInstance> itemInstance = std::make_shared<ItemInstance>(
		weighedCatch->getItemId(), weighedCatch->getCount(), weighedCatch->getAuxValue()
	);
	
	if ((itemInstance->id == Item::fishing_rod_Id && catchType == CatchType::JUNK) || (itemInstance->id == Item::leather_boots_Id)) {
		itemInstance->setAuxValue((int) ((double) itemInstance->getMaxDamage() * ((double) random->nextInt(901) + 100.0) / 1000.0)); // 10% to 100% damage 
	}
	else if (itemInstance->id == Item::fishing_rod_Id && catchType == CatchType::TREASURE) {
		itemInstance->setAuxValue((int)((double) itemInstance->getMaxDamage() * ((double)random->nextInt(251) / 1000.0))); // 0% to 25% damage
		EnchantmentHelper::enchantItem(random, itemInstance, 30);
	}
	else if (itemInstance->id == Item::bow_Id) {
		itemInstance->setAuxValue((int)((double) itemInstance->getMaxDamage() * ((double)random->nextInt(251) / 1000.0))); // 0% to 25% damage
		EnchantmentHelper::enchantItem(random, itemInstance, 30);
	}
	else if (itemInstance->id == Item::book_Id) {
		EnchantmentHelper::enchantItem(random, itemInstance, 30);
	}

	return itemInstance;
}

std::shared_ptr<ItemInstance> FishingHelper::getCatch(int luckLevel, int lureLevel, Random* random)
{
	CatchType catchType = getRandCatchType(luckLevel, lureLevel, random);
	CatchWeighedItem* catchWeighedItem = getRandCatch(catchType, random);
	return handleCatch(catchWeighedItem, catchType, random);
}
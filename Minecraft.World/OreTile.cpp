#include "stdafx.h"
#include "OreTile.h"
#include "net.minecraft.world.item.h"
#include "net.minecraft.world.level.h"

OreTile::OreTile(int id) : Tile(id, Material::stone)
{
}

int OreTile::getResource(int data, Random *random, int playerBonusLevel)
{
    if (id == Tile::coal_ore_Id) return Item::coal_Id;
    if (id == Tile::diamond_ore_Id) return Item::diamond_Id;
    if (id == Tile::lapis_ore_Id) return Item::dye_Id;
	if (id == Tile::emerald_ore_Id) return Item::emerald_Id;
	if (id == Tile::quartz_ore_Id) return Item::quartz_Id;
    return id;
}

int OreTile::getResourceCount(Random *random)
{
    if (id == Tile::lapis_ore_Id) return 4 + random->nextInt(5);
    return 1;
}

int OreTile::getResourceCountForLootBonus(int bonusLevel, Random *random)
{
	if (bonusLevel > 0 && id != getResource(0, random, bonusLevel))
	{
		int bonus = random->nextInt(bonusLevel + 2) - 1;
		if (bonus < 0)
		{
			bonus = 0;
		}
		return getResourceCount(random) * (bonus + 1);
	}
	return getResourceCount(random);
}

void OreTile::spawnResources(Level *level, int x, int y, int z, int data, float odds, int playerBonusLevel)
{
	Tile::spawnResources(level, x, y, z, data, odds, playerBonusLevel);

	// also spawn experience if the block is broken
	if (getResource(data, level->random, playerBonusLevel) != id)
	{
		int magicCount = 0;
		if (id == Tile::coal_ore_Id)
		{
			magicCount = Mth::nextInt(level->random, 0, 2);
		}
		else if (id == Tile::diamond_ore_Id)
		{
			magicCount = Mth::nextInt(level->random, 3, 7);
		}
		else if (id == Tile::emerald_ore_Id)
		{
			magicCount = Mth::nextInt(level->random, 3, 7);
		}
		else if (id == Tile::lapis_ore_Id)
		{
			magicCount = Mth::nextInt(level->random, 2, 5);
		}
		else if (id == Tile::quartz_ore_Id)
		{
			magicCount = Mth::nextInt(level->random, 2, 5);
		}
		popExperience(level, x, y, z, magicCount);
	}
}

int OreTile::getSpawnResourcesAuxValue(int data)
{
    // lapis spawns blue dye
    if (id == Tile::lapis_ore_Id) return DyePowderItem::BLUE;
    return 0;
}
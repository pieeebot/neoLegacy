#include "stdafx.h"
#include "net.minecraft.world.item.h"
#include "Tile.h"
#include "FurnaceRecipes.h"
#include "SmoothStoneBrickTile.h"

FurnaceRecipes *FurnaceRecipes::instance = nullptr;

void FurnaceRecipes::staticCtor()
{
	FurnaceRecipes::instance = new FurnaceRecipes();
}

FurnaceRecipes *FurnaceRecipes::getInstance()
{
	return instance;
}

FurnaceRecipes::FurnaceRecipes()
{
	addFurnaceRecipy(Tile::iron_ore_Id, new ItemInstance(Item::iron_ingot), .7f);
	addFurnaceRecipy(Tile::gold_ore_Id, new ItemInstance(Item::gold_ingot), 1);
	addFurnaceRecipy(Tile::diamond_ore_Id, new ItemInstance(Item::diamond), 1);
	addFurnaceRecipy(Tile::sand_Id, new ItemInstance(Tile::glass), .1f);
	addFurnaceRecipy(Item::porkchop_Id, new ItemInstance(Item::cooked_porkchop), .35f);
	addFurnaceRecipy(Item::beef_Id, new ItemInstance(Item::cooked_beef), .35f);
	addFurnaceRecipy(Item::rabbit_Id, new ItemInstance(Item::cooked_rabbit), .35f);
	addFurnaceRecipy(Item::mutton_Id, new ItemInstance(Item::cooked_mutton), .35f);
	addFurnaceRecipy(Item::chicken_Id, new ItemInstance(Item::cooked_chicken), .35f);
	addFurnaceRecipy(Item::fish_Id, new ItemInstance(Item::cooked_fish), .35f);
	addFurnaceRecipy(new ItemInstance(Item::raw_fish, 1, 1), new ItemInstance(Item::cooked_fish, 1, 1), .35f); // salmon

	addFurnaceRecipy(Tile::cobblestone_Id, new ItemInstance(Tile::stone, 1, 0), .1f);
	addFurnaceRecipy(Tile::stonebrick_Id, new ItemInstance(Tile::stoneBrick, 1 , SmoothStoneBrickTile::TYPE_CRACKED), .1f);
	addFurnaceRecipy(Item::clay_Id, new ItemInstance(Item::brick), .3f);
	addFurnaceRecipy(Tile::clay_Id, new ItemInstance(Tile::clayHardened), .35f);
	addFurnaceRecipy(Tile::cactus_Id, new ItemInstance(Item::dye, 1, DyePowderItem::GREEN), .2f);
	addFurnaceRecipy(Tile::log_Id, new ItemInstance(Item::coal, 1, CoalItem::CHAR_COAL), .15f);
	addFurnaceRecipy(Tile::emerald_ore_Id, new ItemInstance(Item::emerald), 1);
	addFurnaceRecipy(Item::potato_Id, new ItemInstance(Item::baked_potato), .35f);
	addFurnaceRecipy(Tile::netherrack_Id, new ItemInstance(Item::netherbrick), .1f);
	addFurnaceRecipy(new ItemInstance(Tile::sponge, 1, 1), new ItemInstance(Tile::sponge, 1, 0), .15f);

	// special silk touch related recipes:
	addFurnaceRecipy(Tile::coal_ore_Id, new ItemInstance(Item::coal), .1f);
	addFurnaceRecipy(Tile::redstone_ore_Id, new ItemInstance(Item::redstone), .7f);
	addFurnaceRecipy(Tile::lapis_ore_Id, new ItemInstance(Item::dye, 1, DyePowderItem::BLUE), .2f);
	addFurnaceRecipy(Tile::quartz_ore_Id, new ItemInstance(Item::nether_quartz), .2f);

	addFurnaceRecipy(Tile::log2_Id, new ItemInstance(Item::coal, 1, CoalItem::CHAR_COAL), .15f);
}

void FurnaceRecipes::addFurnaceRecipy(int itemId, ItemInstance *result, float value)
{
	//recipies->put(itemId, result);
	recipies[itemId]=result;
	recipeValue[result->id] = value;
}

void FurnaceRecipes::addFurnaceRecipy(ItemInstance* input, ItemInstance* result, float value)
{
	int key = input->id | (input->getAuxValue() << 12);
	recipies[key] = result;
	recipeValue[result->id] = value;
}

bool FurnaceRecipes::isFurnaceItem(int itemId, int data)
{
	int key = itemId | (data << 12);
	if (recipies.find(key) != recipies.end()) return true;
	return recipies.find(itemId) != recipies.end();
}

ItemInstance* FurnaceRecipes::getResult(int itemId, int data)
{
	int key = itemId | (data << 12);
	auto it = recipies.find(key);
	if (it != recipies.end()) return it->second;
	it = recipies.find(itemId);
	if (it != recipies.end()) return it->second;
	return nullptr;
}

unordered_map<int, ItemInstance *> *FurnaceRecipes::getRecipies()
{
	return &recipies;
}

float FurnaceRecipes::getRecipeValue(int itemId)
{
    auto it = recipeValue.find(itemId);
    if (it != recipeValue.end())
	{
		return it->second;
	}
	return 0.0f;
}
//package net.minecraft.world.item.crafting;

//import net.minecraft.world.item.*;
//import net.minecraft.world.level.tile.Tile;
#include "stdafx.h"
#include "net.minecraft.world.item.h"
#include "Tile.h"
#include "Recipy.h"
#include "Recipes.h"
#include "ArmorRecipes.h"

// 4J-PB - adding "" on the end of these so we can detect it
wstring ArmorRecipes::shapes[][4] = 
{
	{L"XXX", //
	L"X X",L""},//

	{L"X X", //
	L"XXX",//
	L"XXX",L""},//

	{L"XXX", //
	L"X X",//
	L"X X",L""},//

	{L"X X",//
	L"X X",L""},//
};

/*
ArmorRecipes::map[5] = 
{
	{Item::leather, Tile::fire, Item::iron_ingot, Item::diamond, Item::gold_ingot}, 
	{Item::helmet_cloth, Item::chainmail_helmet, Item::iron_helmet, Item::diamond_helmet, Item::golden_helmet}, 
	{Item::chestplate_cloth, Item::chainmail_chestplate, Item::iron_chestplate, Item::diamond_chestplate, Item::golden_chestplate}, 
	{Item::leggings_cloth, Item::chainmail_leggings, Item::iron_leggings, Item::diamond_leggings, Item::golden_leggings}, 
	{Item::boots_cloth, Item::chainmail_boots, Item::iron_boots, Item::diamond_boots, Item::golden_boots}, 
};
*/

void ArmorRecipes::_init()
{
	map = new vector <Object *> [MAX_ARMOUR_RECIPES];

	// 4J-PB - removing the chain armour, since we show all possible recipes in the xbox game, and it's not one you can make
	ADD_OBJECT(map[0],Item::leather);
//	ADD_OBJECT(map[0],Tile::fire);
	ADD_OBJECT(map[0],Item::iron_ingot);
	ADD_OBJECT(map[0],Item::diamond);
	ADD_OBJECT(map[0],Item::gold_ingot);

	ADD_OBJECT(map[1],Item::leather_helmet);
//	ADD_OBJECT(map[1],Item::chainmail_helmet);
	ADD_OBJECT(map[1],Item::iron_helmet);
	ADD_OBJECT(map[1],Item::diamond_helmet);
	ADD_OBJECT(map[1],Item::golden_helmet);

	ADD_OBJECT(map[2],Item::leather_chestplate);
//	ADD_OBJECT(map[2],Item::chainmail_chestplate);
	ADD_OBJECT(map[2],Item::iron_chestplate);
	ADD_OBJECT(map[2],Item::diamond_chestplate);
	ADD_OBJECT(map[2],Item::golden_chestplate);

	ADD_OBJECT(map[3],Item::leather_leggings);
//	ADD_OBJECT(map[3],Item::chainmail_leggings);
	ADD_OBJECT(map[3],Item::iron_leggings);
	ADD_OBJECT(map[3],Item::diamond_leggings);
	ADD_OBJECT(map[3],Item::golden_leggings);

	ADD_OBJECT(map[4],Item::leather_boots);
//	ADD_OBJECT(map[4],Item::chainmail_boots);
	ADD_OBJECT(map[4],Item::iron_boots);
	ADD_OBJECT(map[4],Item::diamond_boots);
	ADD_OBJECT(map[4],Item::golden_boots);
}

// 4J-PB added for quick equip in the inventory
ArmorRecipes::_eArmorType ArmorRecipes::GetArmorType(int iId) 
{
	switch(iId)
	{
	case Item::leather_helmet_Id:	
	case Item::chainmail_helmet_Id:	
	case Item::iron_helmet_Id:		
	case Item::diamond_helmet_Id:						
	case Item::golden_helmet_Id:	
		return eArmorType_Helmet;
		break;

	case Item::leather_chestplate_Id:
	case Item::chainmail_chestplate_Id:
	case Item::iron_chestplate_Id:	
	case Item::diamond_chestplate_Id:
	case Item::golden_chestplate_Id:
	case Item::elytra_Id:

		return eArmorType_Chestplate;
		break;

	case Item::leather_leggings_Id:
	case Item::chainmail_leggings_Id:	
	case Item::iron_leggings_Id:
	case Item::diamond_leggings_Id:
	case Item::golden_leggings_Id:
		return eArmorType_Leggings;
		break;

	case Item::leather_boots_Id:		
	case Item::chainmail_boots_Id:	
	case Item::iron_boots_Id:
	case Item::diamond_boots_Id:
	case Item::golden_boots_Id:
		return eArmorType_Boots;
		break;
	}

	return eArmorType_None;
}

void ArmorRecipes::addRecipes(Recipes *r) 
{
	wchar_t wchTypes[5];
	wchTypes[4]=0;

	for (unsigned int m = 0; m < map[0].size(); m++) 
	{
		Object *pObjMaterial = map[0].at(m);

		for (int t=0; t<MAX_ARMOUR_RECIPES-1; t++) 
		{
			Item *target = map[t+1].at(m)->item;

			wchTypes[0]=L'w';
			wchTypes[1]=L'c';
			wchTypes[3]=L'g';
			if(pObjMaterial->GetType()==eType_TILE)
			{
				wchTypes[2]=L't';
				r->addShapedRecipy(new ItemInstance(target), 
					wchTypes,
					shapes[t], 

					L'X', pObjMaterial->tile,
					L'A');
			}
			else
			{
				// must be Item
				wchTypes[2]=L'i';
				r->addShapedRecipy(new ItemInstance(target), 
					wchTypes,
					shapes[t], 

					L'X', pObjMaterial->item,
					L'A');
			}
		}
	}
}


#include "stdafx.h"
#include "net.minecraft.world.item.h"
#include "Tile.h"
#include "Recipy.h"
#include "Recipes.h"
#include "ToolRecipies.h"

// 4J-PB - adding "" on the end of these so we can detect it
wstring ToolRecipies::shapes[][4] = 
{
	{L"XXX", //
	L" # ",//
	L" # "},//

	{L"X",//
	L"#",//
	L"#"},//

	{L"XX",//
	L"X#",//
	L" #"},//

	{L"XX",//
	L" #",//
	L" #"},//
};

void ToolRecipies::_init()
{
	map = new vector <Object *> [MAX_TOOL_RECIPES];

	ADD_OBJECT(map[0],Tile::wood);
	ADD_OBJECT(map[0],Tile::cobblestone);
	ADD_OBJECT(map[0],Item::iron_ingot);
	ADD_OBJECT(map[0],Item::diamond);
	ADD_OBJECT(map[0],Item::gold_ingot);

	ADD_OBJECT(map[1],Item::wooden_pickaxe);
	ADD_OBJECT(map[1],Item::stone_pickaxe);
	ADD_OBJECT(map[1],Item::iron_pickaxe);
	ADD_OBJECT(map[1],Item::diamond_pickaxe);
	ADD_OBJECT(map[1],Item::golden_pickaxe);

	ADD_OBJECT(map[2],Item::wooden_shovel);
	ADD_OBJECT(map[2],Item::stone_shovel);
	ADD_OBJECT(map[2],Item::iron_shovel);
	ADD_OBJECT(map[2],Item::diamond_shovel);
	ADD_OBJECT(map[2],Item::golden_shovel);

	ADD_OBJECT(map[3],Item::wooden_axe);
	ADD_OBJECT(map[3],Item::stone_axe);
	ADD_OBJECT(map[3],Item::iron_axe);
	ADD_OBJECT(map[3],Item::diamond_axe);
	ADD_OBJECT(map[3],Item::golden_axe);

	ADD_OBJECT(map[4],Item::wooden_hoe);
	ADD_OBJECT(map[4],Item::stone_hoe);
	ADD_OBJECT(map[4],Item::iron_hoe);
	ADD_OBJECT(map[4],Item::diamond_hoe);
	ADD_OBJECT(map[4],Item::golden_hoe);
}

void ToolRecipies::addRecipes(Recipes *r) 
{
	wchar_t wchTypes[7];
	wchTypes[6]=0;

	for (unsigned int m = 0; m < map[0].size(); m++) 
	{
		Object *pObjMaterial = map[0].at(m);

		for (int t=0; t<MAX_TOOL_RECIPES-1; t++) 
		{
			Item *target = map[t+1].at(m)->item;

			wchTypes[0]=L'w';
			wchTypes[1]=L'c';
			wchTypes[2]=L'i';
			wchTypes[3]=L'c';
			wchTypes[5]=L'g';
			if(pObjMaterial->GetType()==eType_TILE)
			{
				wchTypes[4]=L't';
				r->addShapedRecipy(new ItemInstance(target), 
					wchTypes,
					shapes[t], 

					L'#', Item::stick,
					L'X', pObjMaterial->tile,
					L'T');
			}
			else
			{
				// must be Item
				wchTypes[4]=L'i';
				r->addShapedRecipy(new ItemInstance(target), 
					wchTypes,
					shapes[t], 

					L'#', Item::stick,
					L'X', pObjMaterial->item,
					L'T');
			}
		}
	}
	r->addShapedRecipy(new ItemInstance(static_cast<Item *>(Item::shears)), 
		L"sscig",
        L" #", //
        L"# ", //
		L'#', Item::iron_ingot,
		L'T'
		);
}
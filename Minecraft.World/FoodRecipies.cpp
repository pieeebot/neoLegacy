#include "stdafx.h"
#include "net.minecraft.world.item.h"
#include "DyePowderItem.h"
#include "Tile.h"
#include "Recipy.h"
#include "Recipes.h"
#include "FoodRecipies.h"

void FoodRecipies::addRecipes(Recipes *r) 
{
	// 4J-JEV: Bumped up in the list to avoid a colision with the title.
	r->addShapedRecipy(new ItemInstance(Item::golden_apple, 1, 0), //
		L"ssscicig",
		L"###", //
		L"#X#", //
		L"###", //
		L'#', Item::gold_ingot, L'X', Item::apple,
		L'F');

	r->addShapedRecipy(new ItemInstance(Item::golden_apple, 1, 1), //
		L"sssctcig",
		L"###", //
		L"#X#", //
		L"###", //
		L'#', Tile::goldBlock, L'X', Item::apple,
		L'F');

	r->addShapedRecipy(new ItemInstance(Item::speckled_melon, 1), //
		L"ssscicig",
		L"###", //
		L"#X#", //
		L"###", //

		L'#', Item::gold_nugget, L'X', Item::melon,
		L'F');
	
	r->addShapelessRecipy(new ItemInstance(Item::mushroom_stew), 
		L"ttig", 
		Tile::mushroom_brown, Tile::mushroom_red, Item::bowl,
		L'F');

	r->addShapedRecipy(new ItemInstance(Item::rabbit_stew), 
		L"ssscicictcicig",
		L" 1 ",//s
		L"2X3",//s
		L" 4 ",//s
		L'1', Item::cooked_rabbit,    // ci
		L'2', Item::carrots,          // ci
		L'3', Tile::mushroom_brown,   // ct  
		L'X', Item::potato,           // ci
		L'4', Item::bowl,             // ci
		L'F');

	r->addShapedRecipy(new ItemInstance(Item::rabbit_stew), 
		L"ssscicictcicig",
		L" 1 ",//s
		L"2X3",//s
		L" 4 ",//s
		L'1', Item::cooked_rabbit,    // ci
		L'2', Item::carrots,          // ci
		L'3', Tile::mushroom_red,     // ct  
		L'X', Item::potato,           // ci
		L'4', Item::bowl,             // ci
		L'F');

	
      
	r->addShapedRecipy(new ItemInstance(Item::cookie, 8), //
				L"sczcig",
                L"#X#", //

                L'X', new ItemInstance(Item::dye, 1, DyePowderItem::BROWN),
				L'#', Item::wheat,
				L'F');

	r->addShapedRecipy(new ItemInstance(Tile::melon), //
		L"ssscig",
		L"MMM", //
		L"MMM", //
		L"MMM", //

		L'M', Item::melon,
		L'F');

	r->addShapedRecipy(new ItemInstance(Item::seeds_melon), //
		L"scig",
		L"M", //

		L'M', Item::melon,
		L'F');

	r->addShapedRecipy(new ItemInstance(Item::seeds_pumpkin, 4), //
		L"sctg",
		L"M", //

		L'M', Tile::pumpkin,
		L'F');

	r->addShapelessRecipy(new ItemInstance(Item::pumpkin_pie), //
		L"tiig",
		Tile::pumpkin, Item::sugar, Item::egg,
		L'F');

	r->addShapedRecipy(new ItemInstance(Item::golden_carrot, 1, 0), //
		L"ssscicig",
		L"###", //
		L"#X#", //
		L"###", //

		L'#', Item::gold_nugget, L'X', Item::carrots,
		L'F');

	r->addShapelessRecipy(new ItemInstance(Item::fermented_spider_eye), //
		L"itig",
		Item::spider_eye, Tile::mushroom_brown, Item::sugar,
		L'F');

	r->addShapelessRecipy(new ItemInstance(Item::blaze_powder, 2), //
		L"ig",
		Item::blaze_rod,
		L'F');

	r->addShapelessRecipy(new ItemInstance(Item::magma_cream), //
		L"iig",
		Item::blaze_powder, Item::slime_ball,
		L'F');
}


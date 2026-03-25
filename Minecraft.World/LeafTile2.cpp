#include "stdafx.h"
#include "LeafTile2.h"
#include "net.minecraft.world.level.tile.h"
#include "net.minecraft.world.level.h"
#include "net.minecraft.world.item.h"
#include "net.minecraft.world.entity.player.h"
#include "net.minecraft.stats.h"
#include "net.minecraft.world.level.biome.h"
#include "FoliageColor.h"
#include "IconRegister.h"

const unsigned int LeafTile2::LEAF2_NAMES[LEAF2_NAMES_SIZE] = {
	IDS_TILE_LEAVES_ACACIA, 
	IDS_TILE_LEAVES_DARK_OAK
};

const wstring LeafTile2::TEXTURES[2][2] = { 
	{ L"leaves_acacia", L"leaves_dark_oak" },               // Indice 0: Fancy
	{ L"leaves_acacia_opaque", L"leaves_dark_oak_opaque" }  // Indice 1: Veloce/Opaca
};

LeafTile2::LeafTile2(int id) : LeafTile(id)
{
	// Non serve fare checkBuffer qui, ci pensa già la classe padre LeafTile!
}

Icon *LeafTile2::getTexture(int face, int data)
{
	int type = data & 3; 
	if (type >= LEAF2_NAMES_SIZE) type = 0;
	
	// isSolidRender() in LeafTile restituisce 'true' se la grafica è su Veloce/Opaca.
	// Quindi se è true usiamo l'indice 1, se è false (Trasparente) usiamo l'indice 0.
	int textureSet = isSolidRender(false) ? 1 : 0; 
	
	return icons[textureSet][type];
}

unsigned int LeafTile2::getDescriptionId(int iData)
{
	int type = iData & 3;
	if (type < 0 || type >= LEAF2_NAMES_SIZE) type = 0;
	return LeafTile2::LEAF2_NAMES[type];
}

void LeafTile2::registerIcons(IconRegister *iconRegister)
{
	for (int fancy = 0; fancy < 2; fancy++)
	{
		for (int i = 0; i < 2; i++)
		{
			icons[fancy][i] = iconRegister->registerIcon(TEXTURES[fancy][i]);
		}
	}
}

int LeafTile2::getColor(int data)
{
	// In inventario o in mano, l'Acacia e la Dark Oak usano il verde base
	return FoliageColor::getDefaultColor();
}

int LeafTile2::getColor(LevelSource *level, int x, int y, int z, int data)
{
	// Codice di blending per il colore del bioma (copiato dal tuo LeafTile.cpp)
	int totalRed = 0;
	int totalGreen = 0;
	int totalBlue = 0;

	for (int oz = -1; oz <= 1; oz++)
	{
		for (int ox = -1; ox <= 1; ox++)
		{
			int foliageColor = level->getBiome(x + ox, z + oz)->getFolageColor(); // Attento, nel tuo engine si chiama getFolageColor() senza la 'i'
			totalRed += (foliageColor & 0xff0000) >> 16;
			totalGreen += (foliageColor & 0xff00) >> 8;
			totalBlue += (foliageColor & 0xff);
		}
	}

	return (((totalRed / 9) & 0xFF) << 16) | (((totalGreen / 9) & 0xFF) << 8) | (((totalBlue / 9) & 0xFF));
}

void LeafTile2::playerDestroy(Level *level, shared_ptr<Player> player, int x, int y, int z, int data)
{
	// Se il giocatore usa le cesoie, vogliamo droppare "leaves2" (ID 161) e non "leaves" (ID 18)
	if (!level->isClientSide && player->getSelectedItem() != nullptr && player->getSelectedItem()->id == Item::shears->id)
	{
		player->awardStat(
			GenericStats::blocksMined(id),
			GenericStats::param_blocksMined(id, data, 1)
		);

		popResource(level, x, y, z, std::make_shared<ItemInstance>(Tile::leaves2_Id, 1, data & 3));
	}
	else
	{
		// Altrimenti usa la distruzione standard di TransparentTile
		TransparentTile::playerDestroy(level, player, x, y, z, data);
	}
}

void LeafTile2::spawnResources(Level* level, int x, int y, int z, int data, float odds, int playerBonusLevel) {
	if (!level->isClientSide)
	{
		int chance = 20;
		if (playerBonusLevel > 0)
		{
			chance -= 2 << playerBonusLevel;
			if (chance < 10) chance = 10;
		}

		if (level->random->nextInt(chance) == 0)
		{


			popResource(level, x, y, z, std::make_shared<ItemInstance>(Tile::sapling_Id, 1, (data & 3) + 4));
		}


		if ((data & 3) == 1)
		{
			int appleChance = 200;
			if (playerBonusLevel > 0)
			{
				appleChance -= 10 << playerBonusLevel;
				if (appleChance < 40) appleChance = 40;
			}
			if (level->random->nextInt(appleChance) == 0)
			{
				popResource(level, x, y, z, std::make_shared<ItemInstance>(Item::apple_Id, 1, 0));
			}
		}
	}
}

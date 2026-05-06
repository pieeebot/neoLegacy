#include "stdafx.h"
#include "net.minecraft.world.level.h"
#include "net.minecraft.world.level.tile.piston.h"
#include "net.minecraft.h"
#include "net.minecraft.world.h"
#include "LeafTile.h"

#include "TreeTile2.h"

const unsigned int TreeTile2::TREE_NAMES[TreeTile2::TREE_NAMES_LENGTH] = { IDS_TILE_LOG_ACACIA, IDS_TILE_LOG_DARK_OAK};

const wstring TreeTile2::TREE_STRING_NAMES[TreeTile2::TREE_NAMES_LENGTH] = { L"acacia", L"dark" };

const wstring TreeTile2::TREE_TEXTURES[] = { L"tree_acacia", L"tree_dark" };

TreeTile2::TreeTile2(int id) : RotatedPillarTile(id, Material::wood)
{
}

void TreeTile2::createBlockStateDefinition()
{
	if (!m_blockStateDefinition)
		m_blockStateDefinition = new BlockStateDefinition(this);
}

int TreeTile2::defaultBlockState()
{
	return 0;
}

int TreeTile2::convertBlockStateToLegacyData(BlockState *state)
{
	return state ? (state->value & (MASK_TYPE | MASK_FACING)) : 0;
}

Tile::BlockState TreeTile2::getBlockState(int data)
{
	return Tile::BlockState(data & (MASK_TYPE | MASK_FACING));
}

Tile::BlockState TreeTile2::getBlockState(LevelSource *level, int x, int y, int z)
{
	return Tile::BlockState(level->getData(x, y, z) & (MASK_TYPE | MASK_FACING));
}

int TreeTile2::getResourceCount(Random* random)
{
	return 1;
}

int TreeTile2::getResource(int data, Random* random, int playerBonusLevel)
{
	return Tile::log2_Id;
}

void TreeTile2::onRemove(Level* level, int x, int y, int z, int id, int data)
{
	int r = LeafTile::REQUIRED_WOOD_RANGE;
	int r2 = r + 1;

	if (level->hasChunksAt(x - r2, y - r2, z - r2, x + r2, y + r2, z + r2))
	{
		for (int xo = -r; xo <= r; xo++)
			for (int yo = -r; yo <= r; yo++)
				for (int zo = -r; zo <= r; zo++)
				{
					int t = level->getTile(x + xo, y + yo, z + zo);
					if (t == Tile::leaves_Id || t == Tile::leaves2_Id)
					{
						int currentData = level->getData(x + xo, y + yo, z + zo);
						if ((currentData & LeafTile::UPDATE_LEAF_BIT) == 0)
						{
							level->setData(x + xo, y + yo, z + zo, currentData | LeafTile::UPDATE_LEAF_BIT, Tile::UPDATE_NONE);
						}
					}
				}
	}
}

Icon* TreeTile2::getTypeTexture(int type)
{
	return icons_side[type];
}

Icon* TreeTile2::getTopTexture(int type)
{
	return icons_top[type];
}

int TreeTile2::getWoodType(int data)
{
	return data & MASK_TYPE;
}

shared_ptr<ItemInstance> TreeTile2::getSilkTouchItemInstance(int data)
{
	// fix to avoid getting silktouched sideways logs
	return std::make_shared<ItemInstance>(id, 1, getWoodType(data));
}

void TreeTile2::registerIcons(IconRegister* iconRegister)
{
	for (int i = 0; i < TREE_NAMES_LENGTH; i++)
	{
		icons_side[i] = iconRegister->registerIcon(getIconName() + L"_" + TREE_STRING_NAMES[i]);
		icons_top[i] = iconRegister->registerIcon(getIconName() + L"_" + TREE_STRING_NAMES[i] + L"_top");
	}
}
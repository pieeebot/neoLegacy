#include "stdafx.h"
#include "net.minecraft.world.level.h"
#include "net.minecraft.world.entity.item.h"
#include "SandTile.h"
#include "FireTile.h"
#include "net.minecraft.world.h"

const unsigned int SandTile::SAND_NAMES[SAND_NAMES_LENGTH] = { IDS_TILE_STONE,
IDS_TILE_SAND};

const wstring SandTile::TEXTURE_NAMES[] = { L"sand",
		L"red_sand" };

bool SandTile::instaFall = false;

SandTile::SandTile(int type, bool isSolidRender) : Tile(type, Material::sand, isSolidRender)
{
	icons = nullptr;
}

int SandTile::getSpawnResourcesAuxValue(int data)
{
	if (data < 0 || data >= SAND_NAMES_LENGTH) data = 0;

	return data;
}

void SandTile::onPlace(Level* level, int x, int y, int z)
{
	level->addToTickNextTick(x, y, z, id, getTickDelay(level));
}

void SandTile::neighborChanged(Level* level, int x, int y, int z, int type)
{
	level->addToTickNextTick(x, y, z, id, getTickDelay(level));
}

void SandTile::tick(Level* level, int x, int y, int z, Random* random)
{
	if (!level->isClientSide)
	{
		checkSlide(level, x, y, z);
	}
}

void SandTile::checkSlide(Level* level, int x, int y, int z)
{
	int x2 = x;
	int y2 = y;
	int z2 = z;
	if (isFree(level, x2, y2 - 1, z2) && y2 >= 0)
	{
		int r = 32;

		if (instaFall || !level->hasChunksAt(x - r, y - r, z - r, x + r, y + r, z + r))
		{
			level->removeTile(x, y, z);
			while (isFree(level, x, y - 1, z) && y > 0)
				y--;
			if (y > 0)
			{
				level->setTileAndUpdate(x, y, z, id);
			}
		}
		else if (!level->isClientSide)
		{
			// 4J added - don't do anything just now if we can't create any new falling tiles
			if (!level->newFallingTileAllowed())
			{
				level->addToTickNextTick(x, y, z, id, getTickDelay(level));
				return;
			}

			shared_ptr<FallingTile> e = std::make_shared<FallingTile>(level, x + 0.5f, y + 0.5f, z + 0.5f, id, level->getData(x, y, z));
			falling(e);
			level->addEntity(e);
		}
	}
}

void SandTile::falling(shared_ptr<FallingTile> entity)
{
}

int SandTile::getTickDelay(Level* level)
{
	return 2;
}

bool SandTile::isFree(Level* level, int x, int y, int z)
{
	int t = level->getTile(x, y, z);
	if (t == 0) return true;
	if (t == Tile::fire_Id) return true;
	Material* material = Tile::tiles[t]->material;
	if (material == Material::water) return true;
	if (material == Material::lava) return true;
	return false;
}

void SandTile::onLand(Level* level, int xt, int yt, int zt, int data)
{
}

Icon* SandTile::getTexture(int face, int data)
{
	if (data < 0 || data >= SAND_NAMES_LENGTH)
	{
		data = 0;
	}
	return icons[data];
}

void SandTile::registerIcons(IconRegister* iconRegister)
{
	icons = new Icon * [SAND_NAMES_LENGTH];

	for (int i = 0; i < SAND_NAMES_LENGTH; i++)
	{
		icons[i] = iconRegister->registerIcon(TEXTURE_NAMES[i]);
	}
}
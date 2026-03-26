#include "stdafx.h"
#include "net.minecraft.world.level.h"
#include "net.minecraft.world.h"
#include "HalfTransparentTile.h"

HalfTransparentTile::HalfTransparentTile(int id, const wstring &tex, Material *material, bool allowSame) : Tile(id,material,isSolidRender())
{
	this->allowSame = allowSame;
	this->texture = tex;
}

bool HalfTransparentTile::isSolidRender(bool isServerLevel)
{
	return false;
}

bool HalfTransparentTile::shouldRenderFace(LevelSource *level, int x, int y, int z, int face)
{
    int id = level->getTile(x, y, z);
	if (this->id == 95) // Tile::stained_glass
	{
		bool isBlocking = level->isTopSolidBlocking(x, y, z);
		if (face == 0 && level->isSolidBlockingTile(x, y + 1, z) != isBlocking) return true; // Facing::DOWN
		else if (face == 1 && level->isTopSolidBlocking(x, y - 1, z) != isBlocking) return true; // Facing::UP
		else if (face == 2 && level->isSolidBlockingTile(x, y, z + 1) != isBlocking) return true; // Facing::NORTH
		else if (face == 3 && level->isSolidBlockingTile(x, y, z - 1) != isBlocking) return true; // Facing::SOUTH
		else if (face == 4 && level->isSolidBlockingTile(x + 1, y, z) != isBlocking) return true; // Facing::WEST
		else if (face == 5 && level->isSolidBlockingTile(x - 1, y, z) != isBlocking) return true; // Facing::EAST
		else if (face == 6 && level->isSolidBlockingTile(x, y, z) != isBlocking) return true; // not really a direction? is this supposed to be here?
	}

    if (!allowSame && id == this->id) return false;
    return Tile::shouldRenderFace(level, x, y, z, face);
}

bool HalfTransparentTile::blocksLight()
{
	return false;
}

void HalfTransparentTile::registerIcons(IconRegister *iconRegister)
{
	icon = iconRegister->registerIcon(texture);
}
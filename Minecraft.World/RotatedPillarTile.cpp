#include "stdafx.h"
#include "net.minecraft.h"
#include "IconRegister.h"
#include "RotatedPillarTile.h" 

RotatedPillarTile::RotatedPillarTile(int id, Material *material) : Tile(id, material)
{
}

RotatedPillarTile::RotatedPillarTile(int id, Material *material, const wstring &iconNameStr) : Tile(id, material)
{
	setIconName(iconNameStr);
}

int RotatedPillarTile::getRenderShape()
{
	return Tile::SHAPE_TREE;
}

int RotatedPillarTile::getPlacedOnFaceDataValue(Level *level, int x, int y, int z, int face, float clickX, float clickY, float clickZ, int itemValue)
{
	int type = itemValue & MASK_TYPE;
	int facing = 0;

	switch (face)
	{
	case Facing::NORTH:
	case Facing::SOUTH:
		facing = FACING_Z;
		break;
	case Facing::EAST:
	case Facing::WEST:
		facing = FACING_X;
		break;
	case Facing::UP:
	case Facing::DOWN:
		facing = FACING_Y;
		break;
	}

	return type | facing;
}

Icon *RotatedPillarTile::getTexture(int face, int data)
{
	int dir = data & MASK_FACING;
	int type = data & MASK_TYPE;

	if (dir == FACING_Y && (face == Facing::UP || face == Facing::DOWN))
	{
		return getTopTexture(type);
	}
	else if (dir == FACING_X && (face == Facing::EAST || face == Facing::WEST))
	{
		return getTopTexture(type);
	}
	else if (dir == FACING_Z && (face == Facing::NORTH || face == Facing::SOUTH))
	{
		return getTopTexture(type);
	}

	return getTypeTexture(type);
}

Icon *RotatedPillarTile::getTypeTexture(int type)
{
	return icon;
}

Icon *RotatedPillarTile::getTopTexture(int type)
{
	return iconTop;
}

void RotatedPillarTile::registerIcons(IconRegister *iconRegister)
{
	icon = iconRegister->registerIcon(getIconName());
	iconTop = iconRegister->registerIcon(getIconName() + L"_top");
}

int RotatedPillarTile::getSpawnResourcesAuxValue(int data)
{
	return data & MASK_TYPE;
}

int RotatedPillarTile::getType(int data)
{
	return data & MASK_TYPE;
}

shared_ptr<ItemInstance> RotatedPillarTile::getSilkTouchItemInstance(int data)
{
	return std::make_shared<ItemInstance>(id, 1, getType(data));
}
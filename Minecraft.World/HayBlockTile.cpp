#include "stdafx.h"
#include "net.minecraft.world.h"
#include "HayBlockTile.h"

HayBlockTile::HayBlockTile(int id) : RotatedPillarTile(id, Material::grass)
{
}

void HayBlockTile::createBlockStateDefinition()
{
	RotatedPillarTile::createBlockStateDefinition();
}

int HayBlockTile::defaultBlockState()
{
	return 0;
}

int HayBlockTile::convertBlockStateToLegacyData(BlockState *state)
{
	return state ? (state->value & MASK_FACING) : 0;
}

Tile::BlockState HayBlockTile::getBlockState(int data)
{
	return Tile::BlockState(data & MASK_FACING);
}

Tile::BlockState HayBlockTile::getBlockState(LevelSource *level, int x, int y, int z)
{
	return Tile::BlockState(level->getData(x, y, z) & MASK_FACING);
}

int HayBlockTile::getRenderShape()
{
	return SHAPE_TREE;
}

Icon *HayBlockTile::getTypeTexture(int type)
{
	return icon;
}

void HayBlockTile::registerIcons(IconRegister *iconRegister)
{
	iconTop = iconRegister->registerIcon(getIconName() + L"_top");
	icon = iconRegister->registerIcon(getIconName() + L"_side");
}
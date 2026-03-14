#include "stdafx.h"
#include "net.minecraft.h"
#include "net.minecraft.world.level.material.h"
#include "net.minecraft.world.h"
#include "RedSandStoneTile.h"

const wstring RedSandStoneTile::TEXTURE_TOP = L"red_sandstone_top";
const wstring RedSandStoneTile::TEXTURE_BOTTOM = L"red_sandstone_bottom";
const wstring RedSandStoneTile::TEXTURE_NAMES[] = { L"red_sandstone_normal", L"red_sandstone_carved", L"red_sandstone_smooth" };

int RedSandStoneTile::SANDSTONE_NAMES[SANDSTONE_BLOCK_NAMES] = {
	IDS_TILE_SANDSTONE, IDS_TILE_SANDSTONE_CHISELED, IDS_TILE_SANDSTONE_SMOOTH
};

RedSandStoneTile::RedSandStoneTile(int id) : Tile(id, Material::stone)
{
	icons = nullptr;
	iconTop = nullptr;
	iconBottom = nullptr;
}

Icon* RedSandStoneTile::getTexture(int face, int data)
{
	if (face == Facing::UP || (face == Facing::DOWN && (data == TYPE_HEIROGLYPHS || data == TYPE_SMOOTHSIDE)))
	{
		return iconTop;
	}
	if (face == Facing::DOWN)
	{
		return iconBottom;
	}
	if (data < 0 || data >= SANDSTONE_TILE_TEXTURE_COUNT) data = 0;
	return icons[data];
}

int RedSandStoneTile::getSpawnResourcesAuxValue(int data)
{
	return data;
}

void RedSandStoneTile::registerIcons(IconRegister* iconRegister)
{
	icons = new Icon * [SANDSTONE_TILE_TEXTURE_COUNT];

	for (int i = 0; i < SANDSTONE_TILE_TEXTURE_COUNT; i++)
	{
		icons[i] = iconRegister->registerIcon(TEXTURE_NAMES[i]);
	}

	iconTop = iconRegister->registerIcon(TEXTURE_TOP);
	iconBottom = iconRegister->registerIcon(TEXTURE_BOTTOM);
}
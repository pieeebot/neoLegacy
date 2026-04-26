#include "stdafx.h"
#include "DirtTile.h"
#include "net.minecraft.world.h"
#include "net.minecraft.h"

const unsigned int DirtTile::DIRT_NAMES[DIRT_NAMES_LENGTH] = { IDS_TILE_DIRT,
IDS_TILE_COARSE_DIRT,
IDS_TILE_PODZOL
};

const unsigned int DirtTile::DIRT_DESCRIPTIONS[DIRT_NAMES_LENGTH] = { IDS_DESC_DIRT,
IDS_DESC_COARSE_DIRT,
IDS_DESC_PODZOL
};

const wstring DirtTile::TEXTURE_NAMES[] = { L"dirt", L"coarse_dirt", L"dirt_podzol"};

DirtTile::DirtTile(int id) : Tile(id, Material::dirt)
{
	icons = nullptr;
    podzolTop = nullptr;
    podzolSide = nullptr;
}

unsigned int DirtTile::getDescriptionId(int iData)
{
    if (iData < 0 || iData >= DIRT_NAMES_LENGTH) iData = 0;

    return DIRT_DESCRIPTIONS[iData];
}

int DirtTile::getSpawnResourcesAuxValue(int data)
{

    if (data == DirtTile::PODZOL) data = 0;

    return data;
}

Icon* DirtTile::getTexture(int face, int data)
{
    if (data < 0 || data >= DIRT_NAMES_LENGTH)
        data = 0;

    if (TEXTURE_NAMES[data] == L"dirt_podzol") {
        return (face == Facing::UP) ? podzolTop : podzolSide;
    }

    return icons[data];
}

void DirtTile::registerIcons(IconRegister* iconRegister)
{
    icons = new Icon * [DIRT_NAMES_LENGTH];

    for (int i = 0; i < DIRT_NAMES_LENGTH; i++)
    {
        if (TEXTURE_NAMES[i] == L"dirt_podzol") {
            icons[i] = nullptr;
            podzolTop = iconRegister->registerIcon(L"dirt_podzol_top");
            podzolSide = iconRegister->registerIcon(L"dirt_podzol_side");
        } else {
            icons[i] = iconRegister->registerIcon(TEXTURE_NAMES[i]);
        }
    }
}
#include "stdafx.h"
#include "WoodSlabTile.h"
#include "WoodTile.h"
#include "TreeTile.h"
#include "net.minecraft.world.level.h"
#include "net.minecraft.world.level.biome.h"
#include "net.minecraft.world.item.h"
#include "net.minecraft.stats.h"

const unsigned int WoodSlabTile::SLAB_NAMES[SLAB_NAMES_LENGTH] = {
    IDS_TILE_STONESLAB_OAK,
    IDS_TILE_STONESLAB_SPRUCE,
    IDS_TILE_STONESLAB_BIRCH,
    IDS_TILE_STONESLAB_JUNGLE,
    IDS_TILE_STONESLAB_ACACIA,
    IDS_TILE_STONESLAB_DARK_OAK
};

WoodSlabTile::WoodSlabTile(int id)
    : HalfSlabTile(id, Material::wood) 
{
}

Icon *WoodSlabTile::getTexture(int face, int data)
{
    
    return Tile::wood->getTexture(face, data & TYPE_MASK);
}

int WoodSlabTile::getResource(int data, Random *random, int playerBonusLevel)
{
    return Tile::wooden_slab_Id;
}

shared_ptr<ItemInstance> WoodSlabTile::getSilkTouchItemInstance(int data)
{
    return std::make_shared<ItemInstance>(Tile::woodSlabHalf, 2, data & TYPE_MASK);
}

int WoodSlabTile::getAuxName(int auxValue)
{
    if (auxValue < 0 || auxValue >= SLAB_NAMES_LENGTH)
        auxValue = 0;
    return SLAB_NAMES[auxValue];
}

void WoodSlabTile::registerIcons(IconRegister *iconRegister)
{
    
}
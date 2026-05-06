#include "stdafx.h"
#include "DoublePlantFeature.h"
#include "net.minecraft.world.level.h"
#include "net.minecraft.world.level.tile.h"
#include "TallGrass2.h"

DoublePlantFeature::DoublePlantFeature(bool doUpdate)
    : Feature(doUpdate), m_plantType(1)
{
}

void DoublePlantFeature::setPlantType(int plantType)
{
    m_plantType = plantType;
}

bool DoublePlantFeature::place(Level* level, Random* rand, int x, int y, int z)
{
    bool placed = false;

    for (int i = 0; i < 64; ++i)
    {
        int bx = x + rand->nextInt(8) - rand->nextInt(8);
        int by = y + rand->nextInt(4) - rand->nextInt(4);
        int bz = z + rand->nextInt(8) - rand->nextInt(8);

        if (by >= Level::maxBuildHeight - 1 || by < 1) continue;

        if (level->getTile(bx, by,     bz) != 0) continue;
        if (level->getTile(bx, by + 1, bz) != 0) continue;

        
        if (!static_cast<TallGrass2*>(Tile::tiles[Tile::double_plant_Id])->mayPlace(level, bx, by, bz)) continue;

        
        level->setTileAndData(bx, by,     bz, Tile::double_plant_Id, m_plantType,                    0);
      level->setTileAndData(bx, by + 1, bz, Tile::double_plant_Id, TallGrass2::UPPER_BIT | m_plantType, 0);

        placed = true;
    }

    return placed;
}
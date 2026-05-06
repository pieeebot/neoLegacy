#include "stdafx.h"
#include "net.minecraft.world.level.h"
#include "net.minecraft.world.level.tile.h"
#include "net.minecraft.world.level.levelgen.feature.h"
#include "net.minecraft.world.h"
#include "Dimension.h"

#include "Sapling.h"
#include "SavannaTreeFeature.h"
#include "RoofTreeFeature.h"
#include "MegaPineTreeFeature.h"

#if defined(_WINDOWS64)
#include "../Minecraft.Server/FourKitBridge.h"
#endif

int Sapling::SAPLING_NAMES[SAPLING_NAMES_SIZE] = {
    IDS_TILE_SAPLING_OAK,
    IDS_TILE_SAPLING_SPRUCE,
    IDS_TILE_SAPLING_BIRCH,
    IDS_TILE_SAPLING_JUNGLE,
    IDS_TILE_SAPLING_ACACIA,
    IDS_TILE_SAPLING_DARK_OAK
};

const wstring Sapling::TEXTURE_NAMES[] = {
    L"sapling",
    L"sapling_spruce",
    L"sapling_birch",
    L"sapling_jungle",
    L"sapling_acacia",
    L"sapling_dark_oak"
};

Sapling::Sapling(int id) : Bush(id)
{
    this->updateDefaultShape();
    icons = nullptr;
}

void Sapling::createBlockStateDefinition()
{
    if (!m_blockStateDefinition)
        m_blockStateDefinition = new BlockStateDefinition(this);
}

int Sapling::defaultBlockState()
{
    return 0;
}

int Sapling::convertBlockStateToLegacyData(BlockState *state)
{
    return state ? (state->value & (TYPE_MASK | AGE_BIT)) : 0;
}

Tile::BlockState Sapling::getBlockState(int data)
{
    return Tile::BlockState(data & (TYPE_MASK | AGE_BIT));
}

Tile::BlockState Sapling::getBlockState(LevelSource *level, int x, int y, int z)
{
    return Tile::BlockState(level->getData(x, y, z) & (TYPE_MASK | AGE_BIT));
}

void Sapling::updateDefaultShape()
{
    float ss = 0.4f;
    this->setShape(0.5f - ss, 0, 0.5f - ss, 0.5f + ss, ss * 2, 0.5f + ss);
}

void Sapling::tick(Level *level, int x, int y, int z, Random *random)
{
    if (level->isClientSide) return;

    Bush::tick(level, x, y, z, random);

    if (level->getRawBrightness(x, y + 1, z) >= Level::MAX_BRIGHTNESS - 6)
    {
        if (random->nextInt(7) == 0)
        {
            advanceTree(level, x, y, z, random, true, -1);
        }
    }
}

Icon *Sapling::getTexture(int face, int data)
{
    data = data & TYPE_MASK;
    if (data < 0 || data >= SAPLING_NAMES_SIZE) data = 0;
    return icons[data];
}

void Sapling::advanceTree(Level* level, int x, int y, int z, Random* random, bool naturalGrowth, int entityId)
{
    int data = level->getData(x, y, z);

    if ((data & AGE_BIT) == 0)
    {
        level->setData(x, y, z, data | AGE_BIT, Tile::UPDATE_NONE);
    }
    else
    {
        growTree(level, x, y, z, random, naturalGrowth, entityId);
    }
}

void Sapling::growTree(Level* level, int x, int y, int z, Random* random, bool naturalGrowth, int entityId)
{
    int data = level->getData(x, y, z) & TYPE_MASK;

    Feature *f = nullptr;
    int ox = 0, oz = 0;
    bool multiblock = false;

    if (data == TYPE_EVERGREEN)
    {
        for (ox = 0; ox >= -1; ox--)
        {
            for (oz = 0; oz >= -1; oz--)
            {
                if (isSapling(level, x + ox, y, z + oz, TYPE_EVERGREEN) &&
                    isSapling(level, x + ox + 1, y, z + oz, TYPE_EVERGREEN) &&
                    isSapling(level, x + ox, y, z + oz + 1, TYPE_EVERGREEN) &&
                    isSapling(level, x + ox + 1, y, z + oz + 1, TYPE_EVERGREEN))
                {
                    f = new MegaPineTreeFeature(true, random->nextBoolean());
                    multiblock = true;
                    break;
                }
            }
            if (f) break;
        }

        if (!f)
        {
            ox = oz = 0;
            f = new SpruceFeature(true);
        }
    }
    else if (data == TYPE_BIRCH)
    {
        f = new BirchFeature(true);
    }
    else if (data == TYPE_JUNGLE)
    {
        for (ox = 0; ox >= -1; ox--)
        {
            for (oz = 0; oz >= -1; oz--)
            {
                if (isSapling(level, x + ox, y, z + oz, TYPE_JUNGLE) &&
                    isSapling(level, x + ox + 1, y, z + oz, TYPE_JUNGLE) &&
                    isSapling(level, x + ox, y, z + oz + 1, TYPE_JUNGLE) &&
                    isSapling(level, x + ox + 1, y, z + oz + 1, TYPE_JUNGLE))
                {
                    f = new MegaTreeFeature(true, 10 + random->nextInt(20), TreeTile::JUNGLE_TRUNK, LeafTile::JUNGLE_LEAF);
                    multiblock = true;
                    break;
                }
            }
            if (f) break;
        }

        if (!f)
        {
            ox = oz = 0;
            f = new TreeFeature(true, 4 + random->nextInt(7), TreeTile::JUNGLE_TRUNK, LeafTile::JUNGLE_LEAF, false);
        }
    }
    else if (data == TYPE_ACACIA)
    {
        f = new SavannaTreeFeature(true);
    }
    else if (data == TYPE_DARK_OAK)
    {
        for (ox = 0; ox >= -1; ox--)
        {
            for (oz = 0; oz >= -1; oz--)
            {
                if (isSapling(level, x + ox, y, z + oz, TYPE_DARK_OAK) &&
                    isSapling(level, x + ox + 1, y, z + oz, TYPE_DARK_OAK) &&
                    isSapling(level, x + ox, y, z + oz + 1, TYPE_DARK_OAK) &&
                    isSapling(level, x + ox + 1, y, z + oz + 1, TYPE_DARK_OAK))
                {
                    f = new RoofTreeFeature(true);
                    multiblock = true;
                    break;
                }
            }
            if (f) break;
        }

        if (!f) return;
    }
    else
    {
        f = new TreeFeature(true);
        if (random->nextInt(10) == 0)
        {
            delete f;
            f = new BasicTree(true);
        }
    }

#if defined(_WINDOWS64) && defined(MINECRAFT_SERVER_BUILD)
    int TreeType = data;
    if (data != TYPE_EVERGREEN && data != TYPE_BIRCH && data != TYPE_JUNGLE)
    {
        TreeType = (dynamic_cast<BasicTree*>(f) != nullptr) ? 4 : 5;
    }

    if (FourKitBridge::FireStructureGrow(level->dimension->id, x, y, z, TreeType, !naturalGrowth, entityId))
    {
        delete f;
        f = nullptr;
    }
#endif

    if (multiblock)
    {
        level->setTileAndData(x + ox, y, z + oz, 0, 0, Tile::UPDATE_NONE);
        level->setTileAndData(x + ox + 1, y, z + oz, 0, 0, Tile::UPDATE_NONE);
        level->setTileAndData(x + ox, y, z + oz + 1, 0, 0, Tile::UPDATE_NONE);
        level->setTileAndData(x + ox + 1, y, z + oz + 1, 0, 0, Tile::UPDATE_NONE);
    }
    else
    {
        level->setTileAndData(x, y, z, 0, 0, Tile::UPDATE_NONE);
    }

    if (f == nullptr || !f->place(level, random, x + ox, y, z + oz))
    {
        if (multiblock)
        {
            level->setTileAndData(x + ox, y, z + oz, id, data, Tile::UPDATE_NONE);
            level->setTileAndData(x + ox + 1, y, z + oz, id, data, Tile::UPDATE_NONE);
            level->setTileAndData(x + ox, y, z + oz + 1, id, data, Tile::UPDATE_NONE);
            level->setTileAndData(x + ox + 1, y, z + oz + 1, id, data, Tile::UPDATE_NONE);
        }
        else
        {
            level->setTileAndData(x, y, z, id, data, Tile::UPDATE_NONE);
        }
    }

    if (f) delete f;
}

unsigned int Sapling::getDescriptionId(int iData)
{
    if (iData < 0 || iData >= SAPLING_NAMES_SIZE) iData = 0;
    return Sapling::SAPLING_NAMES[iData];
}

int Sapling::getSpawnResourcesAuxValue(int data)
{
    return data & TYPE_MASK;
}

bool Sapling::isSapling(Level *level, int x, int y, int z, int type)
{
    return (level->getTile(x, y, z) == id) && ((level->getData(x, y, z) & TYPE_MASK) == type);
}

bool Sapling::fertilize(Level *level, int x, int y, int z)
{
    this->advanceTree(level, x, y, z, level->random);
    return true;
}

void Sapling::registerIcons(IconRegister *iconRegister)
{
    icons = new Icon*[SAPLING_NAMES_SIZE];

    for (int i = 0; i < SAPLING_NAMES_SIZE; i++)
    {
        icons[i] = iconRegister->registerIcon(TEXTURE_NAMES[i]);
    }
}

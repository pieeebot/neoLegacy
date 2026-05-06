#pragma once

#include "LeafTile.h"
#include "LeafTile2.h"
#include "Bush.h"

class Random;
class ChunkRebuildData;
class Tile;

class Sapling : public Bush
{	
    friend class Tile;
    friend class ChunkRebuildData;

public:
    static const int TYPE_DEFAULT   = LeafTile::NORMAL_LEAF;
    static const int TYPE_EVERGREEN = LeafTile::EVERGREEN_LEAF;
    static const int TYPE_BIRCH     = LeafTile::BIRCH_LEAF;
    static const int TYPE_JUNGLE    = LeafTile::JUNGLE_LEAF;
    static const int TYPE_ACACIA    = LeafTile2::ACACIA_LEAF + 4;
    static const int TYPE_DARK_OAK  = LeafTile2::DARK_OAK_LEAF + 4;

    static const int SAPLING_NAMES_SIZE = 6;

    static int SAPLING_NAMES[SAPLING_NAMES_SIZE];

private:
    static const wstring TEXTURE_NAMES[];

    Icon **icons;

    static const int TYPE_MASK = 7;
    static const int AGE_BIT   = 8;

protected:
    Sapling(int id);

public:
    virtual void createBlockStateDefinition() override;
    virtual int defaultBlockState() override;
    virtual int convertBlockStateToLegacyData(BlockState *state) override;
    virtual Tile::BlockState getBlockState(LevelSource *level, int x, int y, int z) override;
    virtual Tile::BlockState getBlockState(int data);
    virtual void updateDefaultShape(); // 4J Added override
    virtual void tick(Level *level, int x, int y, int z, Random *random);
    
    virtual Icon *getTexture(int face, int data);
    virtual void advanceTree(Level *level, int x, int y, int z, Random *random,
                             bool naturalGrowth = true, int entityId = -1);
    void growTree(Level *level, int x, int y, int z, Random *random,
                  bool naturalGrowth = true, int entityId = -1);

    virtual unsigned int getDescriptionId(int iData = -1);
    bool isSapling(Level *level, int x, int y, int z, int type);
    
    virtual bool fertilize(Level *level, int x, int y, int z);

protected:
    int getSpawnResourcesAuxValue(int data);

public:
    void registerIcons(IconRegister *iconRegister);
};
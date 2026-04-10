#pragma once
#include "Biome.h"
#include "MutatedBiome.h"

class SavannaBiome : public Biome
{
public:
    SavannaBiome(int id);

    virtual Feature *getTreeFeature(Random *random);
    virtual int getFolageColor() const override;
    virtual int getGrassColor() const override;
    //virtual int getWaterColor() override;
    virtual Feature *getFlowerFeature(Random *random, int x, int y, int z) override;
    virtual int getRandomDoublePlantType(Random *random) override;
    virtual void decorate(Level *level, Random *random, int xo, int zo) override;
};

class MutatedSavannaBiome : public MutatedBiome
{
public:
    MutatedSavannaBiome(int id, Biome* baseBiome);

    virtual void buildSurfaceAtDefault(Level *level, Random *random, byte* chunkBlocks, int x, int z, double noiseVal) override;
    virtual void decorate(Level *level, Random *random, int xo, int zo) override;
};
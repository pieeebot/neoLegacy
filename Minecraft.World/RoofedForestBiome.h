#pragma once
#include "Biome.h"

class RoofedForestBiome : public Biome
{
public:
    RoofedForestBiome(int id);
    virtual Feature* getTreeFeature(Random* random) override;
   // virtual void decorate(Level* level, Random* random, int xo, int zo) override;
    virtual int getGrassColor() override;
    virtual int getFolageColor() override;
};
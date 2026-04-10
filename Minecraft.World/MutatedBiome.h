#pragma once
#include "Biome.h"

class MutatedBiome : public Biome
{
public:
    
    MutatedBiome(int id, Biome* baseBiome);
    virtual ~MutatedBiome();

    virtual Feature* getTreeFeature(Random* random) override;
    virtual void decorate(Level* level, Random* rand, int xo, int zo) override;
    virtual int getGrassColor() const override;
    virtual int getFolageColor() const override;
    virtual float getCreatureProbability() const override;
    virtual bool isSame(const Biome* other) const override;
    virtual int getTemperatureCategory() const override;
    virtual Feature* getFlowerFeature(Random* random, int x, int y, int z) override;
    virtual int getRandomDoublePlantType(Random* random) override;
    virtual void buildSurfaceAt(Level* level, Random* random, ChunkPrimer* primer, int x, int z, double noiseVal) override;

    Biome* getBaseBiome() const { return m_baseBiome; }

protected:
    Biome* m_baseBiome;
};

#pragma once
#include "Biome.h"
#include "OreFeature.h"
#include "SpruceFeature.h"

class ExtremeHillsBiome : public Biome
{
private:
    static const bool GENERATE_EMERALD_ORE = true;
	Feature *silverfishFeature;
	int type;
	Feature *taigaFeature;

    static constexpr int TYPE_NORMAL  = 0;
    static constexpr int TYPE_TREES   = 1;
    static constexpr int TYPE_MUTATED = 2;
public:
    
    ExtremeHillsBiome(int id);           

    ExtremeHillsBiome(int id, bool extraTrees);
    ~ExtremeHillsBiome();

    Feature* getTreeFeature(Random* random) override;
    void decorate(Level* level, Random* random, int xo, int zo) override;
    void buildSurfaceAtDefault(Level* level, Random* random, byte* chunkBlocks, int x, int z, double noiseVal) override;
    Biome* mutateHills(Biome* baseBiome);
    Biome* createMutatedBiome(int id);


};
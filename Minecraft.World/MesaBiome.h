#pragma once
#include "Biome.h"
#include "PerlinNoise.h"
#include "Random.h"
#include <cstdint>

class MesaBiome : public Biome
{
public:
    
    static constexpr int BAND_HARDENED_CLAY = 255; 
    static constexpr int BAND_WHITE         = 0;
    static constexpr int BAND_ORANGE        = 1;
    static constexpr int BAND_YELLOW        = 4;
    static constexpr int BAND_BROWN         = 12;
    static constexpr int BAND_RED           = 14;
    static constexpr int BAND_SILVER        = 8;

    
    static constexpr int64_t INVALID_SEED = -1LL;

    
    MesaBiome(int id, bool mesaPlateau, bool hasTrees);
    virtual ~MesaBiome();

    
    

    virtual void     decorate(Level* level, Random* random,
                              int xo, int zo) override;

    virtual Feature* getTreeFeature(Random* random) override;

    virtual void buildSurfaceAtDefault(Level* level, Random* random,
                                       byte* chunkBlocks, byte* chunkData,
                                       int x, int z, double noiseVal) override;


private:
    
    void initBands(int64_t seed);

  
    int  getBandColor(int x, int y, int z);

    
    bool     isMesaPlateau;
    bool     hasTrees;

    
    int64_t  lastSeed;

    
    byte     clayBands[64];

    
    PerlinNoise* clayBandsOffsetNoise;

    
    PerlinNoise* pillarNoise;
    PerlinNoise* pillarRoofNoise;
};
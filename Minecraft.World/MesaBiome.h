#pragma once
#include "Biome.h"
#include "PerlinSimplexNoise.h"
#include "Random.h"
#include "Level.h"
#include "Feature.h"
#include <cstdint>

struct BandEntry {
    int blockId;
    int blockData;
};

class MesaBiome : public Biome
{
public:
    static constexpr int BAND_COUNT         = 64;

    static constexpr int BAND_WHITE         =  0;
    static constexpr int BAND_ORANGE        =  1;
    static constexpr int BAND_YELLOW        =  4;
    static constexpr int BAND_BROWN         = 12;
    static constexpr int BAND_RED           = 14;
    static constexpr int BAND_SILVER        =  8;

    static constexpr int defaultHardenedClayState = 0;
    static constexpr int orangeColoredClayState   = BAND_ORANGE;
    static constexpr int yellowColoredClayState   = BAND_YELLOW;
    static constexpr int brownColoredClayState    = BAND_BROWN;
    static constexpr int redColoredClayState      = BAND_RED;
    static constexpr int whiteColoredClayState    = BAND_WHITE;
    static constexpr int silverColoredClayState   = BAND_SILVER;

    static constexpr int64_t INVALID_SEED = 0LL;

    MesaBiome(int id, bool isMesaPlateau, bool hasTrees);
    virtual ~MesaBiome();

    virtual void      decorate(Level* level, Random* random, int xo, int zo) override;
    virtual Feature*  getTreeFeature(Random* random) override;
    virtual void      buildSurfaceAtDefault(Level* level, Random* random,
                                            byte* chunkBlocks, byte* chunkData,
                                            int x, int z, double noiseVal) override;
    virtual int       getFolageColor() const override;
    virtual int       getGrassColor()  const override;

    MesaBiome*        createMutatedCopy(int newId);

private:
    void      generateBands(int64_t seed);
    BandEntry getBand(int x, int y, int z);

    bool    isMesaPlateau;  //0x104
    bool    hasTrees;       //0x105

    int64_t lastSeed;       //0xF0/0xF4

    BandEntry*          clayBands;            //0xE8
    PerlinSimplexNoise* pillarNoise;          //0xF8 
    PerlinSimplexNoise* pillarRoofNoise;      //0xFC
    PerlinSimplexNoise* clayBandsOffsetNoise; //0x100
};
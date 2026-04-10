#pragma once
using namespace std;

#include "LevelSource.h"
#include "Mob.h"
#include "WeighedRandom.h"
#include "BlockPos.h"
#include "ChunkPrimer.h"
#include "DoublePlantFeature.h"


class Feature;
class MobCategory;
class BiomeDecorator;
class TreeFeature;
class BasicTree;
class BirchFeature;
class SwampTreeFeature;
class ChunkRebuildData;
class PerlinNoise;
class MutatedBiome;
class ChunkPrimer;

class Biome
{
    friend class ChunkRebuildData;
public:
    static void staticCtor();

    static Biome *biomes[256];

    static Biome *ocean;
    static Biome *plains;
    static Biome *desert;
    static Biome *extremeHills;
    static Biome *forest;
    static Biome *taiga;
    static Biome *swampland;
    static Biome *river;
    static Biome *hell;
    static Biome *sky;
    static Biome *frozenOcean;
    static Biome *frozenRiver;
    static Biome *iceFlats;
    static Biome *iceMountains;
    static Biome *mushroomIsland;
    static Biome *mushroomIslandShore ;
    static Biome *beaches;
    static Biome *desertHills;
    static Biome *forestHills;
    static Biome *taigaHills;
    static Biome *smallerExtremeHills;
    static Biome *jungle;
    static Biome *jungleHills;
    static Biome *jungleEdge;
    static Biome *deepOcean;
    static Biome *stoneBeach;
    static Biome *coldBeach;
    static Biome *birchForest;
    static Biome *birchForestHills ;
    static Biome *roofedForest;
    static Biome *coldTaiga;
    static Biome *coldTaigaHills;
    static Biome *megaTaiga;
    static Biome *megaTaigaHills;
    static Biome *extremeHills_plus;
    static Biome *savanna;
    static Biome *savannaPlateau;
    static Biome *mesa;
    static Biome *mesaPlateauF;
    static Biome *mesaPlateau;
    static Biome *theVoid;
    static Biome *sunflowersPlains;
    static Biome *desertM;
    static Biome *extremeHillsM;
    static Biome *flowerForest;
    static Biome *taigaM;
    static Biome *swamplandM;
    static Biome *iceSpikes;
    static Biome *jungleM;
    static Biome *jungleEdgeM;
    static Biome *birchForestM ;
    static Biome *birchForestHillsM;
    static Biome *roofedForestM;
    static Biome *coldTaigaM;
    static Biome *redwoodTaiga;
    static Biome *redwoodTaigaHills;
    static Biome *extremeHills_plusM;
    static Biome *savannaM;
    static Biome *savannaPlateauM;
    static Biome *mesaBryce;
    static Biome *mesaPlateauFM;
    static Biome *mesaPlateauM;
    
  
    

    static const int BIOME_COUNT = 256 ;

public:
    wstring m_name;
    int color;
    byte topMaterial;
    byte topMaterialData;
    byte material;
    byte materialData;
    int leafColor;
    float depth;
    float scale;
    float temperature;
    float downfall;


    BiomeDecorator *decorator;
    PerlinNoise *m_temperatureNoise;
    PerlinNoise  *GRASS_COLOR_NOISE;
    DoublePlantFeature  *DOUBLE_PLANT_GENERATOR;

    const int id;

    class MobSpawnerData : public WeighedRandomItem
    {
    public:
        eINSTANCEOF mobClass;
        int minCount;
        int maxCount;

        MobSpawnerData(eINSTANCEOF mobClass, int probabilityWeight, int minCount, int maxCount) 
            : WeighedRandomItem(probabilityWeight)
        {
            this->mobClass = mobClass;
            this->minCount = minCount;
            this->maxCount = maxCount;
        }
    };

public:
    vector<MobSpawnerData *> enemies;
    vector<MobSpawnerData *> friendlies;
    vector<MobSpawnerData *> waterFriendlies;
    vector<MobSpawnerData *> friendlies_chicken;
    vector<MobSpawnerData *> friendlies_wolf;
    vector<MobSpawnerData *> friendlies_mushroomcow;
    vector<MobSpawnerData *> ambientFriendlies;

    Biome(int id);
    virtual ~Biome();

    BiomeDecorator *createDecorator();

public:
    Biome *setTemperatureAndDownfall(float temp, float downfall);
    Biome *setDepthAndScale(float depth, float scale);

    bool snowCovered;
    bool _hasRain;

    eMinecraftColour m_grassColor;
    eMinecraftColour m_foliageColor;
    eMinecraftColour m_waterColor;
    eMinecraftColour m_skyColor;

    Biome *setNoRain();

public:
    virtual Feature *getTreeFeature(Random *random);
    virtual Feature *getGrassFeature(Random *random);

protected:
    Biome *setSnowCovered();
    Biome *setName(const wstring &name);
    Biome *setLeafColor(int leafColor);
  
public:    
    virtual Biome *setColor(int color, bool b = false);

    Biome *setLeafFoliageWaterSkyColor(eMinecraftColour grassColor, eMinecraftColour foliageColor, 
                                        eMinecraftColour waterColour, eMinecraftColour skyColour);

public:
    virtual int getSkyColor(float temp);

    vector<MobSpawnerData *> *getMobs(MobCategory *category);

    virtual bool hasSnow();
    virtual bool hasRain();
    virtual bool isHumid();

    
    virtual int getDownfallInt();
    virtual int getTemperatureInt();
    
    virtual float getDownfall();            
    virtual float getTemperature();         
    
    virtual float getTemperature(const BlockPos& pos);
    virtual float getTemperature(int x, int y, int z);

    virtual void decorate(Level *level, Random *random, int xo, int zo);
    
    virtual void buildSurfaceAtDefault(Level *level, Random *random, byte* chunkBlocks, int x, int z, double noiseVal);

    
    virtual void buildSurfaceAtDefault(Level *level, Random *random, byte* chunkBlocks, const BlockPos& pos, double noiseVal);
    
    virtual void buildSurfaceAtDefault(Level *level, Random *random, byte* chunkBlocks, byte* chunkData, int x, int z, double noiseVal);
   
    virtual int getWaterColor();

    public:
    
    void setWaterSkyColor(int waterRGB, int skyRGB) {
        m_waterColor = (eMinecraftColour)waterRGB;   
        m_skyColor = (eMinecraftColour)skyRGB;
    }
    void setDebugName(const wstring& name) { m_name = name; }
    int getSkyColor() const { return m_skyColor; }
    
    virtual int getBaseBiomeId() const { return id; }   
    virtual int getFolageColor() const;               
    virtual bool isSame(const Biome* other) const;
    virtual int getTemperatureCategory() const;
    virtual void buildSurfaceAt(Level* level, Random* random, ChunkPrimer* primer, int x, int z, double noiseVal);

    
    virtual float getCreatureProbability() const;
    virtual int getGrassColor() const;
    virtual Feature *getFlowerFeature(Random *random, int x, int y, int z);
    virtual int getRandomDoublePlantType(Random *random);
    
};
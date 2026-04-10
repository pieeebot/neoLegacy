#pragma once

#include "StructureFeature.h"
#include "StructureStart.h"
#include <vector>
#include <unordered_map>
#include <string>

class Biome;
class Level;
class Random;

class OceanMonumentFeature : public StructureFeature
{
public:
    // static void staticCtor(); // Removed, merged into _init
    std::vector<Biome::MobSpawnerData*> monumentEnemies;
private:
    int spacing;
    int separation;

    void _init();

public:
    vector<Biome::MobSpawnerData*>* getMonumentEnemies();
    OceanMonumentFeature();
    OceanMonumentFeature(std::unordered_map<std::wstring, std::wstring> options);
    ~OceanMonumentFeature();

    virtual std::wstring getFeatureName() override;

    void setLevel(Level* lvl) { this->level = lvl; }

    void prescanNearby(int scanRadiusInGridCells = 8);

protected:
    virtual bool isFeatureChunk(int x, int z, bool bIsSuperflat = false) override;
    virtual StructureStart* createStructureStart(int x, int z) override;

public:
    class MonumentStart : public StructureStart
    {
    public:
        MonumentStart();
        MonumentStart(Level* level, Random* random, int chunkX, int chunkZ);
        
        
        static StructureStart* Create() { return new MonumentStart(); }
        virtual EStructureStart GetType() override { return eStructureStart_Monument; }
        
       
    };

};
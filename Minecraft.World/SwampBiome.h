#pragma once
#include "Biome.h"
#include "FlowerFeature.h"
#include "Tile.h"
#include "Rose.h"
class LevelSource;

class SwampBiome : public Biome
{
	// 4J Stu - No idea why this is protected in Java
//protected:
public:
	SwampBiome(int id);

public:
	virtual Feature *getTreeFeature(Random *random);
	virtual void buildSurfaceAtDefault(Level *level, Random *random, byte* chunkBlocks, int x, int z, double noiseVal) override;
	virtual Feature* getFlowerFeature(Random* random, int x, int y, int z) override{return new FlowerFeature(Tile::red_flower_Id, Rose::BLUE_ORCHID);}
	// 4J Stu - Not using these any more
    //virtual int getGrassColor();
    //virtual int getFolageColor();
};
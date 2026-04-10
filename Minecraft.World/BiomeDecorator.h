#pragma once

class Level;
class Random;
class Biome;
class Feature;

class BiomeDecorator
{
	friend class DesertBiome;
	friend class ForestBiome;
	friend class PlainsBiome;
	friend class SwampBiome;
	friend class TaigaBiome;
	friend class MushroomIslandBiome;
	friend class BeachBiome;
	friend class SavannaBiome;
	friend class JungleBiome;
	friend class FlowerForestBiome;
protected:
	Level *level;
	Random *random;
	int xo;
	int zo;
	Biome *biome;

public:
	BiomeDecorator(Biome *biome);

	void decorate(Level *level, Random *random, int xo, int zo);

public:
	Feature *clayFeature;
	Feature *sandFeature;
	Feature *gravelFeature;
	Feature *dirtOreFeature;
	Feature *gravelOreFeature;
	Feature *coalOreFeature;
	Feature *ironOreFeature;
	Feature *goldOreFeature;
	Feature *redStoneOreFeature;
	Feature *diamondOreFeature;
	Feature *lapisOreFeature;
	Feature *graniteOreFeature;
	Feature *dioriteOreFeature;
	Feature *andesiteOreFeature;
	Feature *yellowFlowerFeature;
	Feature *roseFlowerFeature;
	Feature *brownMushroomFeature;
	Feature *redMushroomFeature;
	Feature *hugeMushroomFeature;
	Feature *reedsFeature;
	Feature *cactusFeature;
	Feature *waterlilyFeature;
	Feature *blueOrchidFeature;
	Feature *alliumFeature;
	Feature *azureBluetFeature;
	Feature *oxeyeDaisyFeature;
	Feature *tulipRedFeature;
	Feature *tulipOrangeFeature;
	Feature *tulipWhiteFeature;
	Feature *tulipPinkFeature;
	Feature *doublePlantFeature;

	int doublePlantCount;
	int waterlilyCount;
	int treeCount;
	int flowerCount;
	int grassCount;
	int deadBushCount;
	int mushroomCount;
	int reedsCount;
	int cactusCount;
	int gravelCount;
	int sandCount;
	int clayCount;
	int hugeMushrooms;
	bool liquids;
	int blueOrchidCount;
	int alliumCount;
	int azureBluetCount;
	int oxeyeDaisyCount;

	void _init();

protected:
	virtual void decorate();


	void decorate(int count, Feature *feature);
	void decorateDepthSpan(int count, Feature *feature, int y0, int y1);
	void decorateDepthAverage(int count, Feature *feature, int yMid, int ySpan);
	void decorateOres();
};
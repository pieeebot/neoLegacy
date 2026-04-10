#include "stdafx.h"
#include "net.minecraft.world.level.h"
#include "net.minecraft.world.level.tile.h"
#include "net.minecraft.world.level.levelgen.feature.h"
#include "ExtremeHillsBiome.h"
#include "BiomeDecorator.h"

#include "SpruceFeature.h"
#include "TreeFeature.h"

ExtremeHillsBiome::ExtremeHillsBiome(int id, bool extraTrees) : Biome(id)
{
	silverfishFeature = new OreFeature(Tile::monsterStoneEgg_Id, 8);
	taigaFeature = new SpruceFeature(false);
	friendlies.clear();
	type = 0;
	if (extraTrees)
	{
		decorator->treeCount = 3;
		type = 1;
	}
}

ExtremeHillsBiome::~ExtremeHillsBiome()
{
	delete silverfishFeature;
	delete taigaFeature;
}

Feature *ExtremeHillsBiome::getTreeFeature(Random *random)
{
	if (random->nextInt(3) > 0)
	{
		return new SpruceFeature(false);
	}
	return Biome::getTreeFeature(random);
}

void ExtremeHillsBiome::decorate(Level *level, Random *random, int xo, int zo) {
	Biome::decorate(level, random, xo, zo);

	if (GENERATE_EMERALD_ORE)
	{
		int emeraldCount = 3 + random->nextInt(6);
		for (int d = 0; d < emeraldCount; d++)
		{
			int x = xo + random->nextInt(16);
			int y = random->nextInt((Level::genDepth / 4) - 4) + 4;
			int z = zo + random->nextInt(16);
			int tile = level->getTile(x, y, z);
			if (tile == Tile::stone_Id)
			{
				level->setTileAndData(x, y, z, Tile::emeraldOre_Id, 0, Tile::UPDATE_CLIENTS);
			}
		}
	}

	for (int i = 0; i < 7; i++)
	{
		int x = xo + random->nextInt(16);
		int y = random->nextInt(64);
		int z = zo + random->nextInt(16);
		silverfishFeature->place(level, random, x, y, z);
	}
}

void ExtremeHillsBiome::buildSurfaceAtDefault(Level *level, Random *random, byte* chunkBlocks, int x, int z, double noiseVal)
{
    topMaterial = static_cast<byte>(Tile::grass_Id);
    material = static_cast<byte>(Tile::dirt_Id);

    if ((noiseVal < -1.0 || noiseVal > 2.0) && type == 2)
    {
        topMaterial = static_cast<byte>(Tile::gravel_Id);
        material = static_cast<byte>(Tile::gravel_Id);
    }
    else if (noiseVal > 1.0 && type != 1)
    {
        topMaterial = static_cast<byte>(Tile::stone_Id);
        topMaterialData = 0;
        material = static_cast<byte>(Tile::stone_Id);
        materialData = 0;
    }

    Biome::buildSurfaceAtDefault(level, random, chunkBlocks, x, z, noiseVal);
}

Biome* ExtremeHillsBiome::mutateHills(Biome* baseBiome)
{
	this->type = 2; // Mutated type
	this->setColor(baseBiome->color, true);
	this->setName(baseBiome->m_name + L" M");
	this->setDepthAndScale(baseBiome->depth, baseBiome->scale);
	this->setTemperatureAndDownfall(baseBiome->temperature, baseBiome->downfall);
	return this;
}

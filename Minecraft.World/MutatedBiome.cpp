#include "stdafx.h"
#include "MutatedBiome.h"
#include "BiomeDecorator.h"
#include "Level.h"
#include "Random.h"
#include "ChunkPrimer.h"

MutatedBiome::MutatedBiome(int id,  Biome* baseBiome)
    : Biome(id), m_baseBiome(baseBiome)
{
    if (!baseBiome) return;

    color = baseBiome->color;
    topMaterial = baseBiome->topMaterial;
    topMaterialData = baseBiome->topMaterialData;
    material = baseBiome->material;
    materialData = baseBiome->materialData;
    leafColor = baseBiome->leafColor;
    depth = baseBiome->depth + 0.1f;
    scale = baseBiome->scale + 0.2f;
    temperature = baseBiome->temperature;
    downfall = baseBiome->downfall;
    _hasRain = baseBiome->_hasRain;
    snowCovered = baseBiome->snowCovered;
    
    friendlies = baseBiome->friendlies;
    enemies = baseBiome->enemies;
    waterFriendlies = baseBiome->waterFriendlies;
    friendlies_chicken = baseBiome->friendlies_chicken;
    friendlies_wolf = baseBiome->friendlies_wolf;
    friendlies_mushroomcow = baseBiome->friendlies_mushroomcow;
    ambientFriendlies = baseBiome->ambientFriendlies;

    m_waterColor = baseBiome->m_waterColor;
    m_skyColor = baseBiome->m_skyColor;
    m_grassColor = baseBiome->m_grassColor;
    m_foliageColor = baseBiome->m_foliageColor;
    m_name = baseBiome->m_name + L" Mutated";

    
    if (decorator && baseBiome->decorator) {
        decorator->treeCount = baseBiome->decorator->treeCount;
        decorator->grassCount = baseBiome->decorator->grassCount;
        decorator->flowerCount = baseBiome->decorator->flowerCount;
        decorator->hugeMushrooms = baseBiome->decorator->hugeMushrooms;
        
    }
}

MutatedBiome::~MutatedBiome() {}

Feature* MutatedBiome::getTreeFeature(Random* random) {
    return m_baseBiome ? m_baseBiome->getTreeFeature(random) : Biome::getTreeFeature(random);
}

void MutatedBiome::decorate(Level* level, Random* rand, int xo, int zo) {
    if (m_baseBiome)
        m_baseBiome->decorate(level, rand, xo, zo);
    else
        Biome::decorate(level, rand, xo, zo);
}

int MutatedBiome::getGrassColor() const {
    return m_baseBiome ? m_baseBiome->getGrassColor() : Biome::getGrassColor();
}

int MutatedBiome::getFolageColor() const {
    return m_baseBiome ? m_baseBiome->getFolageColor() : Biome::getFolageColor();
}

float MutatedBiome::getCreatureProbability() const
{
    return m_baseBiome ? m_baseBiome->getCreatureProbability() : Biome::getCreatureProbability();
}

bool MutatedBiome::isSame(const Biome* other) const {
    return m_baseBiome ? m_baseBiome->isSame(other) : Biome::isSame(other);
}

int MutatedBiome::getTemperatureCategory() const {
    return m_baseBiome ? m_baseBiome->getTemperatureCategory() : Biome::getTemperatureCategory();
}

Feature* MutatedBiome::getFlowerFeature(Random* random, int x, int y, int z) {
    return m_baseBiome ? m_baseBiome->getFlowerFeature(random, x, y, z) : Biome::getFlowerFeature(random, x, y, z);
}

int MutatedBiome::getRandomDoublePlantType(Random* random) {
    return m_baseBiome ? m_baseBiome->getRandomDoublePlantType(random) : Biome::getRandomDoublePlantType(random);
}

void MutatedBiome::buildSurfaceAt(Level* level, Random* random, ChunkPrimer* primer, int x, int z, double noiseVal) {
    if (m_baseBiome)
        m_baseBiome->buildSurfaceAt(level, random, primer, x, z, noiseVal);
    else
        Biome::buildSurfaceAt(level, random, primer, x, z, noiseVal);
}
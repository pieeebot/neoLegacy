#include "stdafx.h"
#include "OceanMonumentFeature.h"
#include "OceanMonumentPieces.h"
#include "net.minecraft.world.level.h"
#include "net.minecraft.world.level.biome.h"
#include "Mth.h"
#include "LevelData.h"


void OceanMonumentFeature::_init()
{
    spacing   = 32; 
    separation = 5;
     monumentEnemies.push_back(new Biome::MobSpawnerData(eTYPE_GUARDIAN, 1, 2, 4));
}
std::vector<Biome::MobSpawnerData*>* OceanMonumentFeature::getMonumentEnemies()
{
    return &monumentEnemies;
}

OceanMonumentFeature::OceanMonumentFeature() : StructureFeature() { _init(); }
OceanMonumentFeature::OceanMonumentFeature(std::unordered_map<std::wstring, std::wstring> options) : StructureFeature() { _init(); }
OceanMonumentFeature::~OceanMonumentFeature() {}

std::wstring OceanMonumentFeature::getFeatureName() { return L"Monument"; }

void OceanMonumentFeature::prescanNearby(int scanRadiusInGridCells)
{
    if (this->level == nullptr || this->level->getLevelData() == nullptr) return;

    int candidates = 0;
    int validated = 0;
    int halfSizeChunks = this->level->getLevelData()->getXZSize() / 2;
    int halfSizeBlocks = (this->level->getLevelData()->getXZSize() * 16) / 2;
    app.DebugPrintf("Ocean Monument pre-scan (HalfSize: %d chunks / %d blocks)\n", halfSizeChunks, halfSizeBlocks);

    for (int gz = -scanRadiusInGridCells; gz <= scanRadiusInGridCells; gz++)
    {
        for (int gx = -scanRadiusInGridCells; gx <= scanRadiusInGridCells; gx++)
        {
            candidates++;
            int64_t chunkSeed = (int64_t)gx * 341873128712LL
                              + (int64_t)gz * 132897987541LL
                              + this->level->getSeed()
                              + 10387313LL;
            Random r(chunkSeed);

            int cx = gx * this->spacing;
            int cz = gz * this->spacing;
            cx += (r.nextInt(this->spacing - this->separation) + r.nextInt(this->spacing - this->separation)) / 2;
            cz += (r.nextInt(this->spacing - this->separation) + r.nextInt(this->spacing - this->separation)) / 2;

            int blockX = cx * 16 + 8;
            int blockZ = cz * 16 + 8;
            
            if (Mth::abs(blockX) > halfSizeBlocks || Mth::abs(blockZ) > halfSizeBlocks)
            {
                continue; 
            }
            
            if (this->isFeatureChunk(cx, cz))
            {
                validated++;
            }
        }
    }

    app.DebugPrintf(" Pre-scan done: %d grid slots checked, %d validated monuments found. \n", candidates, validated);
}

bool OceanMonumentFeature::isFeatureChunk(int x, int z, bool bIsSuperflat)
{
    if (this->level == nullptr || this->level->getBiomeSource() == nullptr) return false;

    int i = x;
    int j = z;

    if (x < 0) x -= this->spacing - 1;
    if (z < 0) z -= this->spacing - 1;

    int k = x / this->spacing;
    int l = z / this->spacing;

    int64_t chunkSeed = (int64_t)k * 341873128712LL + (int64_t)l * 132897987541LL + this->level->getSeed() + 10387313LL;
    Random random(chunkSeed);

    k *= this->spacing;
    l *= this->spacing;

    k += (random.nextInt(this->spacing - this->separation) + random.nextInt(this->spacing - this->separation)) / 2;
    l += (random.nextInt(this->spacing - this->separation) + random.nextInt(this->spacing - this->separation)) / 2;

    if (i == k && j == l)
    {
        Biome* centerBiome = this->level->getBiomeSource()->getBiome(i * 16 + 8, j * 16 + 8);
        
        if (centerBiome == Biome::deepOcean)
        {
            std::vector<Biome*> surrounding;
            surrounding.reserve(5);
            surrounding.push_back(Biome::ocean);
            surrounding.push_back(Biome::deepOcean);
            surrounding.push_back(Biome::river);
            surrounding.push_back(Biome::frozenOcean);
            surrounding.push_back(Biome::frozenRiver);

            if (this->level->getBiomeSource()->containsOnly(i * 16 + 8, j * 16 + 8, 29, surrounding))
            {
                app.DebugPrintf("Placed Ocean Monument in valid biome at (%d, %d), (%d, %d)\n",
                    i, j, i * 16 + 8, j * 16 + 8);
                app.AddTerrainFeaturePosition(eTerrainFeature_OceanMonument, i, j);
                return true;
            }
        }
    }

    return false;
}

StructureStart* OceanMonumentFeature::createStructureStart(int x, int z)
{
    if (this->level == nullptr) return nullptr;
    return new MonumentStart(level, random, x, z);
}

OceanMonumentFeature::MonumentStart::MonumentStart() {}

OceanMonumentFeature::MonumentStart::MonumentStart(Level* level, Random* random, int chunkX, int chunkZ)
    : StructureStart(chunkX, chunkZ)
{
    random->setSeed(level->getSeed());
    int64_t i = random->nextLong();
    int64_t j = random->nextLong();
    int64_t k = (int64_t)chunkX * i;
    int64_t l = (int64_t)chunkZ * j;
    random->setSeed(k ^ l ^ level->getSeed());

    int startX = chunkX * 16 + 8 - 29;
    int startZ = chunkZ * 16 + 8 - 29;

    int facing = random->nextInt(4) + 2;

    OceanMonumentPieces::MonumentBuilding* building =
        new OceanMonumentPieces::MonumentBuilding(random, startX, startZ, facing);
    pieces.push_back(building);

    calculateBoundingBox();
}
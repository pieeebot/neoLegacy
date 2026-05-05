#include "stdafx.h"
#include "net.minecraft.world.level.h"
#include "net.minecraft.world.level.storage.h"
#include "net.minecraft.world.level.biome.h"
#include "net.minecraft.world.level.newbiome.layer.h"
#include "System.h"
#include "BiomeSource.h"
#include "../Minecraft.Client/Minecraft.h"
#include "../Minecraft.Client/ProgressRenderer.h"

void BiomeSource::_init()
{
    layer = nullptr;
    zoomedLayer = nullptr;
    generatorOptions = L"";
    cache = new BiomeCache(this);
    
    playerSpawnBiomes.push_back(Biome::plains);
    playerSpawnBiomes.push_back(Biome::forest);
    playerSpawnBiomes.push_back(Biome::taiga);
    playerSpawnBiomes.push_back(Biome::forestHills);
    playerSpawnBiomes.push_back(Biome::taigaHills);
    playerSpawnBiomes.push_back(Biome::jungle);
    playerSpawnBiomes.push_back(Biome::jungleHills);
}

void BiomeSource::_init(int64_t seed, LevelType *generator, int xzSize)
{
    _init(); 
    LayerArray layers = Layer::getDefaultLayers(seed, generator);
    if (layers.length >= 2)
    {
        this->layer = layers.data[0];
        this->zoomedLayer = layers.data[1];
        
        
        layers.data = nullptr;
        layers.length = 0;
    }
}

BiomeSource::BiomeSource()
{
    _init();
}

BiomeSource::BiomeSource(int64_t seed, LevelType *generator, int xzSize)
{
    _init(seed, generator, xzSize);
}

BiomeSource::BiomeSource(Level *level)
{
    _init(level->getSeed(), level->getLevelData()->getGenerator(), level->getLevelData()->getXZSize());
}

BiomeSource::~BiomeSource()
{
       if (cache) {
        delete cache;
        cache = nullptr;
    }
    
    if (layer) { layer = nullptr; }
    if (zoomedLayer) {  zoomedLayer = nullptr; }
}

Biome *BiomeSource::getBiome(ChunkPos *cp)
{
    return getBiome(cp->x << 4, cp->z << 4);
}


Biome *BiomeSource::getBiome(int x, int z)
{
    return cache->getBiome(x, z);
}

float BiomeSource::getDownfall(int x, int z) const
{
    return cache->getDownfall(x, z);
}

floatArray BiomeSource::getDownfallBlock(int x, int z, int w, int h) const
{
    floatArray downfalls;
    getDownfallBlock(downfalls, x, z, w, h);
    return downfalls;
}

void BiomeSource::getDownfallBlock(floatArray &downfalls, int x, int z, int w, int h) const
{
    IntCache::releaseAll();
    
    if (downfalls.data == nullptr || downfalls.length < w * h)
    {
        if(downfalls.data != nullptr) delete [] downfalls.data;
        downfalls = floatArray(w * h);
    }

    intArray result = zoomedLayer->getArea(x, z, w, h);
    for (int i = 0; i < w * h; i++)
    {
        float d = static_cast<float>(Biome::biomes[result[i]]->getDownfallInt()) / 65536.0f;
        if (d > 1) d = 1;
        downfalls[i] = d;
    }
}

BiomeCache::Block *BiomeSource::getBlockAt(int x, int y)
{
    return cache->getBlockAt(x, y);
}

float BiomeSource::getTemperature(int x, int y, int z) const
{
    return scaleTemp(cache->getTemperature(x, z), y);
}

float BiomeSource::scaleTemp(float temp, int y ) const
{
    return temp;
}

floatArray BiomeSource::getTemperatureBlock(int x, int z, int w, int h) const
{
    floatArray temperatures;
    getTemperatureBlock(temperatures, x, z, w, h);
    return temperatures;
}

void BiomeSource::getTemperatureBlock(floatArray& temperatures, int x, int z, int w, int h) const
{
    IntCache::releaseAll();
    
    if (temperatures.data == nullptr || temperatures.length < w * h)
    {
        if( temperatures.data != nullptr ) delete [] temperatures.data;
        temperatures = floatArray(w * h);
    }

    intArray result = zoomedLayer->getArea(x, z, w, h);
    for (int i = 0; i < w * h; i++)
    {
        float t = static_cast<float>(Biome::biomes[result[i]]->getTemperatureInt()) / 65536.0f;
        if (t > 1) t = 1;
        temperatures[i] = t;
    }
}

BiomeArray BiomeSource::getRawBiomeBlock(int x, int z, int w, int h) const
{
    BiomeArray biomes;
    getRawBiomeBlock(biomes, x, z, w, h);
    return biomes;
}

void BiomeSource::getRawBiomeIndices(intArray &biomes, int x, int z, int w, int h) const
{
    IntCache::releaseAll();
    intArray result = layer->getArea(x, z, w, h);
    for (int i = 0; i < w * h; i++)
    {
        biomes[i] = result[i];
    }
}

void BiomeSource::getRawBiomeBlock(BiomeArray &biomes, int x, int z, int w, int h) const
{
    IntCache::releaseAll();
    
    if (biomes.data == nullptr || biomes.length < w * h)
    {
        if(biomes.data != nullptr) delete [] biomes.data;
        biomes = BiomeArray(w * h);
    }

    intArray result = layer->getArea(x, z, w, h);
    for (int i = 0; i < w * h; i++)
    {
        biomes[i] = Biome::biomes[result[i]];
#ifndef _CONTENT_PACKAGE
		if(biomes[i] == nullptr)
		{
			app.DebugPrintf("Tried to assign null biome %d\n", result[i]);
			DEBUG_BREAK();
		}
#endif
    }
}

BiomeArray BiomeSource::getBiomeBlock(int x, int z, int w, int h) const
{
    if (w == 16 && h == 16 && (x & 0xf) == 0 && (z & 0xf) == 0)
    {
        return cache->getBiomeBlockAt(x, z);
    }
    BiomeArray biomes;
    getBiomeBlock(biomes, x, z, w, h, true);
    return biomes;
}

void BiomeSource::getBiomeBlock(BiomeArray& biomes, int x, int z, int w, int h, bool useCache) const
{
    IntCache::releaseAll();
    
    if (biomes.data == nullptr || biomes.length < w * h)
    {
        if(biomes.data != nullptr) delete [] biomes.data;
        biomes = BiomeArray(w * h);
    }

    if (useCache && w == 16 && h == 16 && (x & 0xf) == 0 && (z & 0xf) == 0)
    {
        BiomeArray tmp = cache->getBiomeBlockAt(x, z);
        System::arraycopy(tmp, 0, &biomes, 0, w * h);
        delete tmp.data;
    }

    intArray result = zoomedLayer->getArea(x, z, w, h);
    for (int i = 0; i < w * h; i++)
    {
        biomes[i] = Biome::biomes[result[i]];
    }
}

byteArray BiomeSource::getBiomeIndexBlock(int x, int z, int w, int h) const
{
    if (w == 16 && h == 16 && (x & 0xf) == 0 && (z & 0xf) == 0)
    {
        return cache->getBiomeIndexBlockAt(x, z);
    }
    byteArray biomeIndices;
    getBiomeIndexBlock(biomeIndices, x, z, w, h, true);
    return biomeIndices;
}

void BiomeSource::getBiomeIndexBlock(byteArray& biomeIndices, int x, int z, int w, int h, bool useCache) const
{
    IntCache::releaseAll();
    
    if (biomeIndices.data == nullptr || biomeIndices.length < w * h)
    {
        if(biomeIndices.data != nullptr) delete [] biomeIndices.data;
        biomeIndices = byteArray(w * h);
    }

    if (useCache && w == 16 && h == 16 && (x & 0xf) == 0 && (z & 0xf) == 0)
    {
        byteArray tmp = cache->getBiomeIndexBlockAt(x, z);
        System::arraycopy(tmp, 0, &biomeIndices, 0, w * h);
    }

    intArray result = zoomedLayer->getArea(x, z, w, h);
    for (int i = 0; i < w * h; i++)
    {
        biomeIndices[i] = static_cast<byte>(result[i]);
    }
}

bool BiomeSource::containsOnly(int x, int z, int r, vector<Biome *> allowed)
{
    IntCache::releaseAll();
    int x0 = ((x - r) >> 2);
    int z0 = ((z - r) >> 2);
    int x1 = ((x + r) >> 2);
    int z1 = ((z + r) >> 2);

    int w = x1 - x0 + 1;
    int h = z1 - z0 + 1;

    intArray biomes = layer->getArea(x0, z0, w, h);
    for (int i = 0; i < w * h; i++)
    {
        Biome *b = Biome::biomes[biomes[i]];
        if (find(allowed.begin(), allowed.end(), b) == allowed.end()) return false;
    }

    return true;
}

bool BiomeSource::containsOnly(int x, int z, int r, Biome *allowed)
{
    IntCache::releaseAll();
    int x0 = ((x - r) >> 2);
    int z0 = ((z - r) >> 2);
    int x1 = ((x + r) >> 2);
    int z1 = ((z + r) >> 2);

    int w = x1 - x0;
    int h = z1 - z0;
    int biomesCount = w*h;
    intArray biomes = layer->getArea(x0, z0, w, h);
    for (unsigned int i = 0; i < biomesCount; i++)
    {
        Biome *b = Biome::biomes[biomes[i]];
        if (allowed != b) return false;
    }

    return true;
}

TilePos *BiomeSource::findBiome(int x, int z, int r, Biome *toFind, Random *random)
{
    IntCache::releaseAll();
    int x0 = ((x - r) >> 2);
    int z0 = ((z - r) >> 2);
    int x1 = ((x + r) >> 2);
    int z1 = ((z + r) >> 2);

    int w = x1 - x0 + 1;
    int h = z1 - z0 + 1;
    intArray biomes = layer->getArea(x0, z0, w, h);
    TilePos *res = nullptr;
    int found = 0;
    int biomesCount = w*h;
    for (unsigned int i = 0; i < biomesCount; i++)
    {
        int xx = x0 + i % w;
        int zz = z0 + i / w;
        Biome *b = Biome::biomes[biomes[i]];
        if (b == toFind)
        {
            if (res == nullptr || random->nextInt(found + 1) == 0)
            {
                res = new TilePos(xx, 0, zz);
                found++;
            }
        }
    }

    return res;
}

TilePos *BiomeSource::findBiome(int x, int z, int r, vector<Biome *> allowed, Random *random)
{
    IntCache::releaseAll();
    int x0 = ((x - r) >> 2);
    int z0 = ((z - r) >> 2);
    int x1 = ((x + r) >> 2);
    int z1 = ((z + r) >> 2);

    int w = x1 - x0 + 1;
    int h = z1 - z0 + 1;
    MemSect(50);
    intArray biomes = layer->getArea(x0, z0, w, h);
    TilePos *res = nullptr;
    int found = 0;
    for (unsigned int i = 0; i < w * h; i++)
    {
        int xx = (x0 + i % w) << 2;
        int zz = (z0 + i / w) << 2;
        Biome *b = Biome::biomes[biomes[i]];
        if (find(allowed.begin(), allowed.end(), b) != allowed.end())
        {
            if (res == nullptr || random->nextInt(found + 1) == 0)
            {
                delete res;
                res = new TilePos(xx, 0, zz);
                found++;
            }
        }
    }
    MemSect(0);

    return res;
}

void BiomeSource::update()
{
    cache->update();
}
#ifdef __PSVITA__
int64_t BiomeSource::findSeed(LevelType *generator, bool* pServerRunning)
#else
int64_t BiomeSource::findSeed(LevelType *generator)
#endif
{
    Random pr;
    int64_t seed;
    float fracs[256];
    float groupFracs[8];
    bool matchFound = false;
    int tryCount = 0;

    if (generator != nullptr && generator->m_generatorName != L"default")
    {
        return pr.nextLong();
    }

   do {
        seed = pr.nextLong();
        BiomeSource bs(seed, generator);
        
        intArray indices = bs.layer->getArea(-100, -100, 200, 200);
        
        if (indices.data != nullptr && indices.length > 0)
        {
            bs.getFracs(indices, fracs, groupFracs);
            matchFound = bs.getIsMatch(fracs, groupFracs);
            
            
        }

        tryCount++;

        if (tryCount >= 100) 
        {
            matchFound = true;
        }

        
        IntCache::releaseAll();

#ifdef __PSVITA__
    } while (!matchFound && (*pServerRunning));
#else
    } while (!matchFound);
#endif

    return seed;
}

int BiomeSource::getBiomeGroup(int id)
{
    return 0;
}

void BiomeSource::getFracs(intArray indices, float *fracs, float *groupFracs)
{
    for (int i = 0; i < 256; i++) fracs[i] = 0.0f;
    for (int i = 0; i < 8; i++) groupFracs[i] = 0.0f;

    for (int i = 0; i < indices.length; i++)
    {
        int id = indices[i];
        if (id >= 0 && id < 256)
        {
            fracs[id] += 1.0f;
            int group = getBiomeGroup(id);
            if (group >= 0 && group < 8)
            {
                groupFracs[group] += 1.0f;
            }
        }
    }

    float invLen = 1.0f / (float)indices.length;
    for (int i = 0; i < 256; i++) fracs[i] *= invLen;
    for (int i = 0; i < 8; i++) groupFracs[i] *= invLen;
}

bool BiomeSource::getIsMatch(float *fracs, float *groupFracs)
{
    if (fracs[0] + fracs[24] > 0.15f) return false;

    int varietyCount = 0;
    for (int i = 0; i < 8; i++)
    {
        if (groupFracs[i] > 0.0f)
        {
            varietyCount++;
        }
    }

    return varietyCount >= 5;
}
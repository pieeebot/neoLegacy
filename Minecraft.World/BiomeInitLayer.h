#pragma once

#include "Layer.h"

class LevelType;

class BiomeInitLayer : public Layer
{
private:
	
	BiomeArray startBiomes;

	// matching Java GenLayerBiome
	BiomeArray desertBiomes;  
	BiomeArray warmBiomes;    
	BiomeArray coolBiomes;    
	BiomeArray icyBiomes;     

	bool bLegacy1_1;

public:
	BiomeInitLayer(int64_t seed, shared_ptr<Layer> parent, LevelType *levelType);
	virtual ~BiomeInitLayer();
    intArray getArea(int xo, int yo, int w, int h);
};
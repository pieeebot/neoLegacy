#pragma once

#include "Layer.h"

class RareBiomeLayer : public Layer
{
public:
	RareBiomeLayer(int64_t seed, shared_ptr<Layer> parent);
	virtual ~RareBiomeLayer() {}
	virtual intArray getArea(int xo, int yo, int w, int h) override;
};

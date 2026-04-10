#pragma once

#include "Layer.h"

class DeepOceanLayer : public Layer
{
public:
	DeepOceanLayer(int64_t seed, shared_ptr<Layer> parent);
	virtual ~DeepOceanLayer() {}
	virtual intArray getArea(int xo, int yo, int w, int h) override;
};

#pragma once

#include "Layer.h"

class RegionHillsLayer : public Layer
{
private:
    shared_ptr<Layer> riverNoise; // second parent: zoomed river init layer used as noise source

public:
    RegionHillsLayer(int64_t seed, shared_ptr<Layer> parent);
    RegionHillsLayer(int64_t seed, shared_ptr<Layer> parent, shared_ptr<Layer> riverNoise);

    virtual void init(int64_t seed) override;
    virtual intArray getArea(int xo, int yo, int w, int h) override;

private:
    static bool biomesEqualOrMesaPlateau(int a, int b);
};
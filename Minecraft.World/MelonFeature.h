#pragma once
#include "Feature.h"


class MelonFeature : public Feature
{
public:
    virtual bool place(Level *level, Random *random, int x, int y, int z);
};

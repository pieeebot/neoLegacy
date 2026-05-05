#pragma once

#include "Enchantment.h"

class WaterWalkerEnchantment : public Enchantment
{
public:
	WaterWalkerEnchantment(int id, int freq);

	virtual int getMinCost(int level);
	virtual int getMaxCost(int level);
	virtual int getMaxLevel();
};
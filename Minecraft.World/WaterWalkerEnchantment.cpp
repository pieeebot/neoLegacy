#include "WaterWalkerEnchantment.h"

WaterWalkerEnchantment::WaterWalkerEnchantment(int id, int frequency) : Enchantment(id, frequency, EnchantmentCategory::armor_feet)
{
    setDescriptionId(IDS_ENCHANTMENT_WATER_WALKER);
}

int WaterWalkerEnchantment::getMinCost(int level)
{
    return 1 + (level - 1) * 10;
}

int WaterWalkerEnchantment::getMaxCost(int level)
{
    return getMinCost(level) + 15;
}

int WaterWalkerEnchantment::getMaxLevel()
{
    return 3;
}
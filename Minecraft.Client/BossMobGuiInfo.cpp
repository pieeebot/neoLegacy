#include "stdafx.h"
#include "BossMobGuiInfo.h"
#include "../Minecraft.World/BossMob.h"
#include "../Minecraft.World/LevelData.h"

float BossMobGuiInfo::healthProgress[3] = { 0.0f, 0.0f, 0.0f };
int BossMobGuiInfo::displayTicks[3] = { 0, 0, 0 };
wstring BossMobGuiInfo::name[3];
bool BossMobGuiInfo::darkenWorld[3] = { false, false, false };

void BossMobGuiInfo::setBossHealth(shared_ptr<BossMob> boss, bool darkenWorld)
{
    int idx = getIndexFromDimension(boss->getDimension());
    healthProgress[idx] = (float) boss->getHealth() / (float) boss->getMaxHealth();
    displayTicks[idx] = SharedConstants::TICKS_PER_SECOND * 5;
    name[idx] = boss->getAName();
    BossMobGuiInfo::darkenWorld[idx] = darkenWorld;
}

int BossMobGuiInfo::getIndexFromDimension(int dimension)
{
    if (dimension == LevelData::DIMENSION_NETHER) return 1;
    if (dimension == LevelData::DIMENSION_END) return 2;
    return 0;
}
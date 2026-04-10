#pragma once

class BossMob;

class BossMobGuiInfo
{
public:
    static float healthProgress[3];
    static int displayTicks[3];
    static wstring name[3];
    static bool darkenWorld[3];

    static void setBossHealth(shared_ptr<BossMob> boss, bool darkenWorld);
    
    static int  getIndexFromDimension(int dimension);
};
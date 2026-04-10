#include "stdafx.h"
#include "WaterWakeParticle.h"
#include "../Minecraft.World/Random.h"
#include "../Minecraft.World/Mth.h"
#include "../Minecraft.World/JavaMath.h"
#include "../Minecraft.World/net.minecraft.world.level.h"
#include "../Minecraft.World/net.minecraft.world.level.material.h"
#include "../Minecraft.World/net.minecraft.world.level.tile.h"

WaterWakeParticle::WaterWakeParticle(Level *level, double x, double y, double z, double xa, double ya, double za) : Particle(level, x, y, z, 0, 0, 0)
{
    xd *= 0.30000001192092896;
    yd = random->nextDouble() * 0.2 + 0.1;
    zd *= 0.30000001192092896;
    rCol = 1.0f;
    gCol = 1.0f;
    bCol = 1.0f;
    setMiscTex(19);
    setSize(0.01f, 0.01f);
    lifetime = (int)(8.0 / (random->nextDouble() * 0.8 + 0.2));
    gravity = 0.0F;
    xd = xa;
    yd = ya;
    zd = za;
}

void WaterWakeParticle::tick()
{
    xo = x;
    yo = y;
    zo = z;

    move(xd, yd, zd);
    xd *= 0.9800000190734863;
    yd *= 0.9800000190734863;
    zd *= 0.9800000190734863;

    int i = 60 - lifetime;
    float f = (float)i * 0.001f;
    this->setSize(f, f);
    this->setMiscTex(19 + i % 4);

    if (lifetime-- <= 0)
    {
        this->remove();
    }
}
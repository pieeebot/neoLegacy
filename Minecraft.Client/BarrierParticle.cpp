#include "stdafx.h"
#include "BarrierParticle.h"
#include "Minecraft.h"
#include "Tesselator.h"
#include "../Minecraft.World/Item.h"
#include "../Minecraft.World/Icon.h"
#include "../Minecraft.World/net.minecraft.world.level.tile.h"
#include "../Minecraft.World/Facing.h"
#include "../Minecraft.World/Level.h"
#include "../Minecraft.World/JavaMath.h"

void BarrierParticle::init(Level* level, double x, double y, double z, float scale)
{
    xd = yd = zd = 0;

    rCol = gCol = bCol = 1.0f;
    alpha = 1.0f;

    // fixed size
    size = 0.5f * scale;
    oSize = size;

    lifetime = 80;

    gravity = 0.0f;
}

BarrierParticle::BarrierParticle(Level* level,
                                 double x, double y, double z,
                                 double xa, double ya, double za)
    : Particle(level, x, y, z, xa, ya, za)
{
    init(level, x, y, z, 1.0f);

    // set particle texture to barrier texture
    this->setTex(Minecraft::GetInstance()->textures, Tile::barrier->getTexture(Facing::UP));
}

int BarrierParticle::getParticleTexture()
{
    return ParticleEngine::TERRAIN_TEXTURE;
}

void BarrierParticle::render(Tesselator* t, float a, float xa, float ya, float za, float xa2, float za2)
{
    rCol = gCol = bCol = 1.0f;
    alpha = 1.0f;

    float u0 = tex->getU0();
    float u1 = tex->getU1();
    float v0 = tex->getV0();
    float v1 = tex->getV1();
 
    float half = size;

    float px = static_cast<float>(xo + (this->x - xo) * a - xOff);
    float py = static_cast<float>(yo + (this->y - yo) * a - yOff);
    float pz = static_cast<float>(zo + (this->z - zo) * a - zOff);

    float br = SharedConstants::TEXTURE_LIGHTING ? 1.0f : getBrightness(a);
    t->color(br * rCol, br * gCol, br * bCol, alpha);
    t->tex2(getLightColor(a));

    t->vertexUV((double)(px - xa * half - xa2 * half), (double)(py - ya * half), (double)(pz - za * half - za2 * half), (double)u1, (double)v1);
    t->vertexUV((double)(px - xa * half + xa2 * half), (double)(py + ya * half), (double)(pz - za * half + za2 * half), (double)u1, (double)v0);
    t->vertexUV((double)(px + xa * half + xa2 * half), (double)(py + ya * half), (double)(pz + za * half + za2 * half), (double)u0, (double)v0);
    t->vertexUV((double)(px + xa * half - xa2 * half), (double)(py - ya * half), (double)(pz + za * half - za2 * half), (double)u0, (double)v1);
}

void BarrierParticle::tick()
{
    xo = x;
    yo = y;
    zo = z;

    if (++age >= lifetime)
        remove();

    xd = yd = zd = 0;
}
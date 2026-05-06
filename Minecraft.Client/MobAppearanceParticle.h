#pragma once
#include "Particle.h"
#include "Tesselator.h"
#include "../Minecraft.World/Guardian.h"


class MobAppearanceParticle : public Particle
{
public:
    class Provider
    {
    public:
        shared_ptr<Particle> createParticle(
            ePARTICLE_TYPE       type,
            Level*               level,
            double               x,
            double               y,
            double               z,
            arrayWithLength<int> params);
    };

    MobAppearanceParticle(Level* level, double x, double y, double z);
    virtual ~MobAppearanceParticle();

    virtual int          getParticleTexture() override;

    virtual eINSTANCEOF  GetType() override;

    virtual void tick() override;

    virtual void render(
        Tesselator* t,
        float       a,
        float       xa,
        float       ya,
        float       za,
        float       xa2,
        float       za2) override;

private:
    shared_ptr<Guardian> guardianEntity;
};
#pragma once
#include "Particle.h"

class BarrierParticle : public Particle
{
public:
    virtual eINSTANCEOF GetType() { return eType_BARRIERPARTICLE; }

private:
    void init(Level* level, double x, double y, double z, float scale);

public:
    float oSize;
    
    BarrierParticle(Level* level,
                    double x, double y, double z,
                    double xa, double ya, double za);

    virtual int getParticleTexture();

    virtual void render(Tesselator* t, float a, float xa, float ya, float za, float xa2, float za2);
    virtual void tick();
};
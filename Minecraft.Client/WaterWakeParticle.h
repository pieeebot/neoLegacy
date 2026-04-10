#pragma once
#include "Particle.h"

class WaterWakeParticle : public Particle
{
public:
	virtual eINSTANCEOF GetType() { return eType_WAKEPARTICLE; }
	WaterWakeParticle(Level *level, double x, double y, double z, double xa, double ya, double za);
	virtual void tick();
};
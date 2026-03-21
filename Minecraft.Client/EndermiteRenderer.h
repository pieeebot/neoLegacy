#pragma once
#include "MobRenderer.h"

class Endermite;

class EndermiteRenderer : public MobRenderer
{
private:
	//int modelVersion;
	static ResourceLocation ENDERMITE_LOCATION;

public:
	EndermiteRenderer();

protected:
	float getFlipDegrees(shared_ptr<LivingEntity> spider);

public:
	virtual void render(shared_ptr<Entity> _mob, double x, double y, double z, float rot, float a);
	virtual ResourceLocation *getTextureLocation(shared_ptr<Entity> mob);

protected:
	virtual int prepareArmor(shared_ptr<LivingEntity> _silverfish, int layer, float a);
};

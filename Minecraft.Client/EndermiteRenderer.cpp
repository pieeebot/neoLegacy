#include "stdafx.h"
#include "EndermiteRenderer.h"
#include "..\Minecraft.World\net.minecraft.world.entity.monster.h"
#include "EndermiteModel.h"

ResourceLocation EndermiteRenderer::ENDERMITE_LOCATION(TN_MOB_ENDERMITE);

EndermiteRenderer::EndermiteRenderer() : MobRenderer(new EndermiteModel(), 0.3f)
{
}

float EndermiteRenderer::getFlipDegrees(shared_ptr<LivingEntity> spider)
{
	return 180;
}

void EndermiteRenderer::render(shared_ptr<Entity> _mob, double x, double y, double z, float rot, float a)
{
	MobRenderer::render(_mob, x, y, z, rot, a);
}

ResourceLocation *EndermiteRenderer::getTextureLocation(shared_ptr<Entity> mob)
{
    return &ENDERMITE_LOCATION;
}

int EndermiteRenderer::prepareArmor(shared_ptr<LivingEntity> _silverfish, int layer, float a)
{
	return -1;
}
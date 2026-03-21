#include "stdafx.h"
#include "EndermiteModel.h"
#include "Cube.h"
#include "..\Minecraft.World\Mth.h"
#include "ModelPart.h"


const int EndermiteModel::BODY_SIZES[BODY_COUNT][3] = {
	{ 4, 3, 2 }, 
	{ 6, 4, 5 }, 
	{ 3, 3, 1 }, 
	{ 1, 2, 1 }
};


const int EndermiteModel::BODY_TEXS[BODY_COUNT][2] = {
	{ 0, 0 }, 
	{ 0, 5 }, 
	{ 0, 14 }, 
	{ 0, 18 }
};

EndermiteModel::EndermiteModel()
{
	bodyParts = ModelPartArray(BODY_COUNT);
	float placement = -3.5f;
	
	for (unsigned int i = 0; i < bodyParts.length; i++)
	{
		bodyParts[i] = new ModelPart(this, BODY_TEXS[i][0], BODY_TEXS[i][1]);
		bodyParts[i]->addBox(BODY_SIZES[i][0] * -0.5f, 0, BODY_SIZES[i][2] * -0.5f, BODY_SIZES[i][0], BODY_SIZES[i][1], BODY_SIZES[i][2]);
		bodyParts[i]->setPos(0.0f, 24.0f - static_cast<float>(BODY_SIZES[i][1]), placement);
		zPlacement[i] = placement;
		
		if (i < bodyParts.length - 1)
		{
			placement += (BODY_SIZES[i][2] + BODY_SIZES[i + 1][2]) * .5f;
		}
	}

	

	
	for (unsigned int i = 0; i < bodyParts.length; i++)
	{
		bodyParts[i]->compile(1.0f/16.0f);
	}
}

int EndermiteModel::modelVersion()
{
	return 38;
}

void EndermiteModel::render(shared_ptr<Entity> entity, float time, float r, float bob, float yRot, float xRot, float scale, bool usecompiled)
{
	setupAnim(time, r, bob, yRot, xRot, scale, entity);

	for (unsigned int i = 0; i < bodyParts.length; i++)
	{
		bodyParts[i]->render(scale, usecompiled);
	}
}

void EndermiteModel::setupAnim(float time, float r, float bob, float yRot, float xRot, float scale, shared_ptr<Entity> entity, unsigned int uiBitmaskOverrideAnim)
{
	for (unsigned int i = 0; i < bodyParts.length; i++)
	{
		
		bodyParts[i]->yRot = Mth::cos(bob * .9f + i * .15f * PI) * PI * .01f * (1 + abs(static_cast<int>(i) - 2));
		bodyParts[i]->x = Mth::sin(bob * .9f + i * .15f * PI) * PI * .1f * abs(static_cast<int>(i) - 2);
	}
}
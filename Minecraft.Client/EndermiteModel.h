#pragma once

#include "Model.h"

class EndermiteModel : public Model
{

private:
	static const int BODY_COUNT = 4;

private:
	ModelPartArray bodyParts;
	float zPlacement[BODY_COUNT];

	static const int BODY_SIZES[BODY_COUNT][3];
	static const int BODY_TEXS[BODY_COUNT][2];

public:
	EndermiteModel();

	int modelVersion();
	void render(shared_ptr<Entity> entity, float time, float r, float bob, float yRot, float xRot, float scale, bool usecompiled);
	virtual void setupAnim(float time, float r, float bob, float yRot, float xRot, float scale, shared_ptr<Entity> entity, unsigned int uiBitmaskOverrideAnim=0);
};
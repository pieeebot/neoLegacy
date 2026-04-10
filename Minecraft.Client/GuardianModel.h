#pragma once
#include "Model.h"

class ModelPart;

class GuardianModel : public Model
{
public:
    ModelPart *guardianBody;
    ModelPart *guardianEye;
    ModelPart *guardianSpines[12];
    ModelPart *guardianTail[3];

    GuardianModel();

    
    int getTextureHeight() { return 64; }

    virtual void setupAnim(float time, float r, float bob, float yRot, float xRot,
                           float scale, shared_ptr<Entity> entity,
                           unsigned int uiBitmaskOverrideAnim = 0) override;

    virtual void render(shared_ptr<Entity> entity, float time, float r, float bob,
                        float yRot, float xRot, float scale, bool usecompiled) override;
};
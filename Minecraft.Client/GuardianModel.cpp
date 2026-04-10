#include "stdafx.h"
#include "GuardianModel.h"
#include "..\Minecraft.World\Mth.h"
#include "ModelPart.h"
#include "..\Minecraft.World\Guardian.h"



GuardianModel::GuardianModel() : Model()
{
    texWidth  = 64;
    texHeight = 64;

    
    guardianBody = new ModelPart(this);
    guardianBody->texOffs(0,  0)->addBox(-6.0f, 10.0f, -8.0f, 12, 12, 16);
    guardianBody->texOffs(0, 28)->addBox(-8.0f, 10.0f, -6.0f,  2, 12, 12);

   
    ModelPart *rightSide = new ModelPart(this);
    rightSide->texOffs(0, 28)->mirror()->addBox(6.0f, 10.0f, -6.0f, 2, 12, 12);
    guardianBody->addChild(rightSide);

    guardianBody->texOffs(16, 40)->addBox(-6.0f,  8.0f, -6.0f, 12, 2, 12);
    guardianBody->texOffs(16, 40)->addBox(-6.0f, 22.0f, -6.0f, 12, 2, 12);

    
    for (int i = 0; i < 12; i++)
    {
        guardianSpines[i] = new ModelPart(this, 0, 0);
        guardianSpines[i]->addBox(-1.0f, -4.5f, -1.0f, 2, 9, 2);
        guardianBody->addChild(guardianSpines[i]);
    }

   
    guardianEye = new ModelPart(this, 8, 0);
    guardianEye->addBox(-1.0f, 15.0f, 0.0f, 2, 2, 1);
    guardianBody->addChild(guardianEye);

   
    guardianTail[0] = new ModelPart(this, 40, 0);
    guardianTail[0]->addBox(-2.0f, 14.0f, 7.0f, 4, 4, 8);

    guardianTail[1] = new ModelPart(this, 0, 54);
    guardianTail[1]->addBox(0.0f, 14.0f, 0.0f, 3, 3, 7);

    guardianTail[2] = new ModelPart(this);
    guardianTail[2]->texOffs(41, 32)->addBox(0.0f, 14.0f, 0.0f, 2, 2, 6);
    guardianTail[2]->texOffs(25, 19)->addBox(1.0f, 10.5f, 3.0f, 1, 9, 9);

    guardianBody->addChild(guardianTail[0]);
    guardianTail[0]->addChild(guardianTail[1]);
    guardianTail[1]->addChild(guardianTail[2]);
}



void GuardianModel::setupAnim(float time, float r, float bob, float yRot, float xRot,
                               float scale, shared_ptr<Entity> entity,
                               unsigned int uiBitmaskOverrideAnim)
{
    shared_ptr<Guardian> guardian = dynamic_pointer_cast<Guardian>(entity);
    if (!guardian) return;

   
    float afloat[]  = { 1.75f, 0.25f, 0.0f, 0.0f, 0.5f, 0.5f,  0.5f,  0.5f, 1.25f, 0.75f, 0.0f, 0.0f };
    float afloat1[] = { 0.0f,  0.0f,  0.0f, 0.0f, 0.25f,1.75f, 1.25f, 0.75f, 0.0f,  0.0f,  0.0f, 0.0f };
    float afloat2[] = { 0.0f,  0.0f,  0.25f,1.75f, 0.0f, 0.0f, 0.0f,  0.0f,  0.0f,  0.0f,  0.75f,1.25f};
    float afloat3[] = { 0.0f,  0.0f,  8.0f, -8.0f,-8.0f, 8.0f, 8.0f, -8.0f,  0.0f,  0.0f,  8.0f,-8.0f };
    float afloat4[] = {-8.0f, -8.0f, -8.0f, -8.0f, 0.0f, 0.0f, 0.0f,  0.0f,  8.0f,  8.0f,  8.0f, 8.0f };
    float afloat5[] = { 8.0f, -8.0f,  0.0f,  0.0f,-8.0f,-8.0f, 8.0f,  8.0f,  8.0f, -8.0f,  0.0f, 0.0f };

    // Body orientation
    guardianBody->yRot = yRot / (180.0f / (float)PI);
    guardianBody->xRot = xRot / (180.0f / (float)PI);


    guardianEye->z = -8.25f;

    if (guardian->hasTargetedEntity())
    {
        shared_ptr<LivingEntity> target = guardian->getTargetedEntity();
        if (target)
        {
           
            double d0 = (guardian->y + guardian->getEyeHeight())
                      - (target->y   + target->getEyeHeight());
            guardianEye->y = (d0 > 0.0) ? 0.0f : 1.0f;

          
            double dx = target->x - guardian->x;
            double dz = target->z - guardian->z;
            float  targetYaw = (float)(atan2(dz, dx) * 180.0 / PI) - 90.0f;
            float  diff      = Mth::wrapDegrees(targetYaw - guardian->yRot);

            float clamped = diff / 45.0f;
            if (clamped < -2.0f) clamped = -2.0f;
            if (clamped >  2.0f) clamped =  2.0f;
            guardianEye->x = clamped;
        }
    }
    else
    {
        guardianEye->y = 0.0f;
        guardianEye->x = 0.0f;
    }


    float f1 = (1.0f - guardian->getSpikesAnimation(0.0f)) * 0.55f;

    for (int i = 0; i < 12; ++i)
    {
        guardianSpines[i]->xRot = (float)PI * afloat[i];
        guardianSpines[i]->yRot = (float)PI * afloat1[i];
        guardianSpines[i]->zRot = (float)PI * afloat2[i];

        float wave = 1.0f + Mth::cos(bob * 1.5f + (float)i) * 0.01f - f1;
        guardianSpines[i]->x = afloat3[i] * wave;
        guardianSpines[i]->y = 16.0f + afloat4[i] * wave;
        guardianSpines[i]->z = afloat5[i] * wave;
    }

 
    float tailAnim = guardian->getTailAnimation(0.0f);

    guardianTail[0]->yRot = Mth::sin(tailAnim) * (float)PI * 0.05f;

    guardianTail[1]->yRot = Mth::sin(tailAnim) * (float)PI * 0.1f;
    guardianTail[1]->x   = -1.5f;
    guardianTail[1]->y   =  0.5f;
    guardianTail[1]->z   = 14.0f;

    guardianTail[2]->yRot = Mth::sin(tailAnim) * (float)PI * 0.15f;
    guardianTail[2]->x   =  0.5f;
    guardianTail[2]->y   =  0.5f;
    guardianTail[2]->z   =  6.0f;
}



void GuardianModel::render(shared_ptr<Entity> entity, float time, float r, float bob,
                            float yRot, float xRot, float scale, bool usecompiled)
{
    setupAnim(time, r, bob, yRot, xRot, scale, entity);
    
    guardianBody->render(scale, usecompiled);
}
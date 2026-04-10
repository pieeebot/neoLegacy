#pragma once
#include "MobRenderer.h"

class GuardianModel;

class GuardianRenderer : public MobRenderer
{
private:
    GuardianModel *guardianModel;

    static ResourceLocation GUARDIAN_LOCATION;
    static ResourceLocation GUARDIAN_ELDER_LOCATION;    
    static ResourceLocation GUARDIAN_BEAM_LOCATION;

    
    void getInterpolatedPos(shared_ptr<LivingEntity> entity, double yOffset,
                            float partialTicks,
                            double &outX, double &outY, double &outZ);

public:
    GuardianRenderer(Model *model, float shadow);

    
  

virtual bool shouldRender(shared_ptr<Entity> mob, float camX, float camY, float camZ) override;

    virtual void render(shared_ptr<Entity> mob, double x, double y, double z,
                        float rot, float a) override;

protected:
    
    

    virtual void setupRotations(shared_ptr<LivingEntity> mob, float bob,
                                float bodyRot, float a) override;

    
    virtual void renderModel(shared_ptr<LivingEntity> mob, float wp, float ws, float bob,
                         float headRotMinusBodyRot, float headRotx, float scale) override;
    virtual ResourceLocation *getTextureLocation(shared_ptr<Entity> mob) override;
};
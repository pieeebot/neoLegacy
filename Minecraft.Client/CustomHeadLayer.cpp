#include "stdafx.h"
#include "CustomHeadLayer.h"
#include "LivingEntityRenderer.h"
#include "ModelPart.h"
#include "SkullTileRenderer.h"
#include "TileRenderer.h"
#include "EntityRenderDispatcher.h"
#include "Minecraft.h"
#include "../Minecraft.World/ItemInstance.h"
#include "../Minecraft.World/Item.h"
#include "../Minecraft.World/Tile.h"
#include "../Minecraft.World/SkullItem.h"
#include "../Minecraft.World/SkullTileEntity.h"
#include "../Minecraft.World/LivingEntity.h"
#include "../Minecraft.World/Facing.h"


CustomHeadLayer::CustomHeadLayer(ModelPart* headPart, LivingEntityRenderer* parentRenderer)
    : headPart(headPart), parentRenderer(parentRenderer)
{
}

int CustomHeadLayer::colorsOnDamage()
{
    return 1;
}

void CustomHeadLayer::render(shared_ptr<LivingEntity> mob,
                              float wp, float ws, float bob,
                              float headRot, float headRotX,
                              float scale, bool useCompiled)
{
    shared_ptr<ItemInstance> helmet = mob->getArmor(3);
    if (!helmet) return;

    Item* item = helmet->getItem();
    if (!item) return;

    if (mob->instanceof(eTYPE_PLAYER))
    {
        _SkinAdjustments adj;
        mob->getSkinAdjustments(&adj);
        if ((adj.data[0] & 0x100) != 0) return;
    }

    bool isBaby  = mob->isBaby();
    // FIX: Riconosciamo correttamente lo SkullItem invece dell'ID del Blocco
    bool isSkull = (dynamic_cast<SkullItem*>(item) != nullptr);

    glPushMatrix();
   
    if (isBaby)
        glTranslatef(0.0f, 0.2f, 0.0f);

    headPart->translateTo(0.0625f);
    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

    if (isSkull)
    {
        glScalef(1.1875f, -1.1875f, -1.1875f);

        if (isBaby)
            glTranslatef(0.0f, 0.0625f, 0.0f);

        if (SkullTileRenderer::instance)
        {
            int     skullType = helmet->getAuxValue() & 0xF;
            wstring extra     = L"";
            if (helmet->hasTag() && helmet->getTag()->contains(L"SkullOwner"))
                extra = helmet->getTag()->getString(L"SkullOwner");

            SkullTileRenderer::instance->renderSkull(
                //Skull on armor stand is slightly lowered
                -0.5f, -0.05f, -0.5f,
                Facing::UP,
                180.0f,
                skullType,
                extra
            );
        }
    }
    else if (item->id < 256)
    {
        glTranslatef(0.0f, -0.25f, 0.0f);
        glRotatef(180.0f, 0.0f, 1.0f, 0.0f);
        glScalef(0.625f, -0.625f, -0.625f);

        if (isBaby)
            glTranslatef(0.0f, 0.1875f, 0.0f);

        glRotatef(90.0f, 0.0f, 1.0f, 0.0f);

        Minecraft* mc = Minecraft::GetInstance();
        if (mc)
        {
            auto* iihr = mc->getItemInHandRenderer();
            if (iihr)
                iihr->renderItem(mob, helmet, 0, true);
        }
    }

    glPopMatrix();
}
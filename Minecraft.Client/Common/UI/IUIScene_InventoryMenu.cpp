#include "stdafx.h"
#include "IUIScene_InventoryMenu.h"
#include "..\..\..\Minecraft.World\net.minecraft.world.inventory.h"
#include "..\..\Minecraft.h"
#include "..\..\MultiPlayerLocalPlayer.h"

IUIScene_AbstractContainerMenu::ESceneSection IUIScene_InventoryMenu::GetSectionAndSlotInDirection(ESceneSection eSection, ETapState eTapDirection, int* piTargetX, int* piTargetY)
{
    Minecraft* pMinecraft = Minecraft::GetInstance();

    ESceneSection newSection = eSection;
    int xOffset = 0;
    int yOffset = 0;

    //auto containerData = this->m_containerData;

    switch (eSection)
    {
    case eSectionInventoryInventory:
        if (eTapDirection == eTapStateDown)
        {
            newSection = eSectionInventoryUsing;
        }
        else if (eTapDirection == eTapStateUp)
        {
            if (app.GetGameSettings(getPad(), eGameSetting_ClassicCrafting) && *piTargetX >= 3)
            {
                if (*piTargetX < 7) {
                    newSection = eSectionInventoryCraftingGrid;
                    xOffset = 5;
                }
                else {
                    newSection = eSectionInventoryCraftingResult;
                }
            }
            else
            {
                newSection = eSectionInventoryArmor;
            }
        }
        break;

    case eSectionInventoryUsing:
        if (eTapDirection == eTapStateUp)
        {
            newSection = eSectionInventoryInventory;
        }
        else if (eTapDirection == eTapStateDown)
        {
            if (app.GetGameSettings(getPad(), eGameSetting_ClassicCrafting) && *piTargetX >= 3)
            {
                if (*piTargetX < 7) {
                    newSection = eSectionInventoryCraftingGrid;
                    xOffset = 5;
                }
                else {
                    newSection = eSectionInventoryCraftingResult;
                }
            }
            else
            {
                newSection = eSectionInventoryArmor;
            }
        }
        break;

    case eSectionInventoryArmor:
        if (eTapDirection == eTapStateDown)
        {
            newSection = eSectionInventoryInventory;
        }
        else if (eTapDirection == eTapStateUp)
        {
            newSection = eSectionInventoryUsing;
        }
        else if (eTapDirection == eTapStateLeft && app.GetGameSettings(getPad(), eGameSetting_ClassicCrafting))
        {
            newSection = eSectionInventoryCraftingResult;
        }
        else if (eTapDirection == eTapStateRight && app.GetGameSettings(getPad(), eGameSetting_ClassicCrafting))
        {
            newSection = eSectionInventoryCraftingGrid;
            xOffset = 1;
            yOffset = 1;
        }
        break;
    case eSectionInventoryCraftingGrid:
        if (app.GetGameSettings(getPad(), eGameSetting_ClassicCrafting))
        {
            if (eTapDirection == eTapStateUp)
            {
                newSection = eSectionInventoryUsing;
                xOffset = -5;
            }
            else if (eTapDirection == eTapStateDown)
            {
                newSection = eSectionInventoryInventory;
                xOffset = -5;
            }
            else if (eTapDirection == eTapStateLeft)
            {
                newSection = eSectionInventoryArmor;
                yOffset = -1;
            }
            else if (eTapDirection == eTapStateRight)
            {
                newSection = eSectionInventoryCraftingResult;
            }
        }
        break;

    case eSectionInventoryCraftingResult:
        if (app.GetGameSettings(getPad(), eGameSetting_ClassicCrafting))
        {
            if (eTapDirection == eTapStateUp)
            {
                newSection = eSectionInventoryUsing;
                xOffset = -8;
            }
            else if (eTapDirection == eTapStateDown)
            {
                newSection = eSectionInventoryInventory;
                xOffset = -8;
            }
            else if (eTapDirection == eTapStateLeft)
            {
                newSection = eSectionInventoryCraftingGrid;
                //*piTargetX = containerData->getSectionColumns(eSectionInventoryCraftingGrid);
            }
            else if (eTapDirection == eTapStateRight)
            {
                newSection = eSectionInventoryArmor;
            }
        }
        break;
    default:
        assert(false);
        break;
    }

    IUIScene_AbstractContainerMenu::updateSlotPosition(eSection, newSection, eTapDirection, piTargetX, piTargetY, xOffset, yOffset);

    return newSection;
}

int IUIScene_InventoryMenu::getSectionStartOffset(ESceneSection eSection)
{
    int offset = 0;
    switch (eSection)
    {
    case eSectionInventoryArmor:
        offset = InventoryMenu::ARMOR_SLOT_START;
        break;
    case eSectionInventoryInventory:
        offset = InventoryMenu::INV_SLOT_START;
        break;
    case eSectionInventoryUsing:
        offset = InventoryMenu::INV_SLOT_START + 27;
        break;
    case eSectionInventoryCraftingGrid:
        offset = InventoryMenu::CRAFT_SLOT_START;
        break;
    case eSectionInventoryCraftingResult:
        offset = InventoryMenu::RESULT_SLOT;
        break;
    default:
        assert(false);
        break;
    }
    return offset;
}
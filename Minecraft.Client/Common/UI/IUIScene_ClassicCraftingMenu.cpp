#include "stdafx.h"

#include "IUIScene_ClassicCraftingMenu.h"
#include "..\..\..\Minecraft.World\net.minecraft.world.inventory.h"

IUIScene_AbstractContainerMenu::ESceneSection IUIScene_ClassicCraftingMenu::GetSectionAndSlotInDirection(ESceneSection eSection, ETapState eTapDirection, int* piTargetX, int* piTargetY)
{
    ESceneSection newSection = eSection;
    int xOffset = 0;
    int yOffset = 0;

    // Find the new section if there is one
    switch (eSection)
    {
    case eSectionClassicCraftingGrid:
        if (eTapDirection == eTapStateDown)
        {
            newSection = eSectionClassicCraftingInventory;
            xOffset = -1;
        }
        else if (eTapDirection == eTapStateUp)
        {
            newSection = eSectionClassicCraftingHotbar;
            xOffset = -1;
        }
        else if (eTapDirection == eTapStateLeft)
        {
            newSection = eSectionClassicCraftingResult;
        }
        else if (eTapDirection == eTapStateRight)
        {
            newSection = eSectionClassicCraftingResult;
        }
        break;
    case eSectionClassicCraftingResult:
        if (eTapDirection == eTapStateDown)
        {
            newSection = eSectionClassicCraftingInventory;
            xOffset = -7;
        }
        else if (eTapDirection == eTapStateUp)
        {
            newSection = eSectionClassicCraftingHotbar;
            xOffset = -7;
        }
        else if (eTapDirection == eTapStateLeft)
        {
            newSection = eSectionClassicCraftingGrid;
            yOffset = -1;
            *piTargetX = getSectionColumns(eSectionClassicCraftingGrid);
        }
        else if (eTapDirection == eTapStateRight)
        {
            newSection = eSectionClassicCraftingGrid;
            yOffset = -1;
            *piTargetX = 0;
        }
        break;
    case eSectionClassicCraftingInventory:
        if (eTapDirection == eTapStateDown)
        {
            newSection = eSectionClassicCraftingHotbar;
        }
        else if (eTapDirection == eTapStateUp)
        {
            if (*piTargetX < 6)
            {
                newSection = eSectionClassicCraftingGrid;
                xOffset = 1;
            }
            else
            {
                newSection = eSectionClassicCraftingResult;
            }
        }
        break;
    case eSectionClassicCraftingHotbar:
        if (eTapDirection == eTapStateDown)
        {
            if (*piTargetX < 6)
            {
                newSection = eSectionClassicCraftingGrid;
                xOffset = 1;
            }
            else
            {
                newSection = eSectionClassicCraftingResult;
            }
        }
        else if (eTapDirection == eTapStateUp)
        {
            newSection = eSectionClassicCraftingInventory;
        }
        break;
    default:
        assert(false);
        break;
    }

    updateSlotPosition(eSection, newSection, eTapDirection, piTargetX, piTargetY, xOffset, yOffset);

    return newSection;
}

int IUIScene_ClassicCraftingMenu::getSectionStartOffset(ESceneSection eSection)
{
    int offset = 0;

    switch (eSection)
    {
    case eSectionClassicCraftingHotbar:
        offset = CraftingMenu::INV_SLOT_START + 27;
        break;
    case eSectionClassicCraftingInventory:
        offset = CraftingMenu::INV_SLOT_START;
        break;
    case eSectionClassicCraftingGrid:
        offset = CraftingMenu::CRAFT_SLOT_START;
        break;
    case eSectionClassicCraftingResult:
        offset = CraftingMenu::RESULT_SLOT;
        break;
    default:
        assert(false);
        break;
    }

    return offset;
}
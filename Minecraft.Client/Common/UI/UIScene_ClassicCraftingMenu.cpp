#include "stdafx.h"
#include "UI.h"
#include "..\..\..\Minecraft.World\net.minecraft.world.level.tile.entity.h"
#include "..\..\..\Minecraft.World\net.minecraft.world.inventory.h"
#include "..\..\Minecraft.h"
#include "..\..\LocalPlayer.h"
#include "UIScene_ClassicCraftingMenu.h"

UIScene_ClassicCraftingMenu::UIScene_ClassicCraftingMenu(int iPad, void* _initData, UILayer* parentLayer) : UIScene_AbstractContainerMenu(iPad, parentLayer)
{
	// Setup all the Iggy references we need for this scene
	initialiseMovie();

	CraftingPanelScreenInput* initData = static_cast<CraftingPanelScreenInput*>(_initData);

	m_labelTitle.init(app.GetString(IDS_HOW_TO_PLAY_MENU_CRAFTING));

	Minecraft* pMinecraft = Minecraft::GetInstance();
	if (pMinecraft->localgameModes[m_iPad] != nullptr)
	{
		TutorialMode* gameMode = static_cast<TutorialMode*>(pMinecraft->localgameModes[m_iPad]);
		m_previousTutorialState = gameMode->getTutorial()->getCurrentState();
		gameMode->getTutorial()->changeTutorialState(e_Tutorial_State_3x3Crafting_Menu, this);
	}

	//TODO: Add Fireworks check
//	m_labelTitle.init(app.GetString(IDS_HOW_TO_PLAY_MENU_FIREWORKS));
//	FireworksMenu *m_menu = new FireworksMenu(initData->player->inventory, initData->player->level, initData->x, initData->y, initData->z);

	CraftingMenu* m_menu = new CraftingMenu(initData->player->inventory, initData->player->level, initData->x, initData->y, initData->z);

	Initialize(initData->iPad, m_menu, true, CraftingMenu::INV_SLOT_START, eSectionClassicCraftingHotbar, eSectionClassicCraftingMax);

	m_slotListResult.addSlots(CraftingMenu::RESULT_SLOT, 1);
	m_slotListCrafting.addSlots(CraftingMenu::CRAFT_SLOT_START, 9);

	app.SetRichPresenceContext(m_iPad, CONTEXT_GAME_STATE_CRAFTING);

	delete initData;
}

wstring UIScene_ClassicCraftingMenu::getMoviePath()
{
	if (app.GetLocalPlayerCount() > 1)
		return L"ClassicCraftingMenuSplit";
	else
		return L"ClassicCraftingMenu";
}

void UIScene_ClassicCraftingMenu::handleReload()
{
	Initialize(m_iPad, m_menu, true, CraftingMenu::INV_SLOT_START, eSectionClassicCraftingHotbar, eSectionClassicCraftingMax);

	m_slotListResult.addSlots(CraftingMenu::RESULT_SLOT, 1);
	m_slotListCrafting.addSlots(CraftingMenu::CRAFT_SLOT_START, 9);
}

int UIScene_ClassicCraftingMenu::getSectionColumns(ESceneSection eSection)
{
	int cols = 0;
	switch (eSection)
	{
	case eSectionClassicCraftingHotbar:
		cols = 9;
		break;
	case eSectionClassicCraftingInventory:
		cols = 9;
		break;
	case eSectionClassicCraftingGrid:
		cols = 3;
		break;
	case eSectionClassicCraftingResult:
		cols = 1;
		break;
	default:
		assert(false);
		break;
	}
	return cols;
}

int UIScene_ClassicCraftingMenu::getSectionRows(ESceneSection eSection)
{
	int rows = 0;
	switch (eSection)
	{
	case eSectionClassicCraftingHotbar:
		rows = 1;
		break;
	case eSectionClassicCraftingInventory:
		rows = 3;
		break;
	case eSectionClassicCraftingGrid:
		rows = 3;
		break;
	case eSectionClassicCraftingResult:
		rows = 1;
		break;
	default:
		assert(false);
		break;
	}
	return rows;
}

void UIScene_ClassicCraftingMenu::GetPositionOfSection(ESceneSection eSection, UIVec2D* pPosition)
{
	switch (eSection)
	{
	case eSectionClassicCraftingHotbar:
		pPosition->x = m_slotListHotbar.getXPos();
		pPosition->y = m_slotListHotbar.getYPos();
		break;
	case eSectionClassicCraftingInventory:
		pPosition->x = m_slotListInventory.getXPos();
		pPosition->y = m_slotListInventory.getYPos();
		break;
	case eSectionClassicCraftingGrid:
		pPosition->x = m_slotListCrafting.getXPos();
		pPosition->y = m_slotListCrafting.getYPos();
		break;
	case eSectionClassicCraftingResult:
		pPosition->x = m_slotListResult.getXPos();
		pPosition->y = m_slotListResult.getYPos();
		break;
	default:
		assert(false);
		break;
	}
}

void UIScene_ClassicCraftingMenu::GetItemScreenData(ESceneSection eSection, int iItemIndex, UIVec2D* pPosition, UIVec2D* pSize)
{
	UIVec2D sectionSize;
	switch (eSection)
	{
	case eSectionClassicCraftingHotbar:
		sectionSize.x = m_slotListHotbar.getWidth();
		sectionSize.y = m_slotListHotbar.getHeight();
		break;
	case eSectionClassicCraftingInventory:
		sectionSize.x = m_slotListInventory.getWidth();
		sectionSize.y = m_slotListInventory.getHeight();
		break;
	case eSectionClassicCraftingGrid:
		sectionSize.x = m_slotListCrafting.getWidth();
		sectionSize.y = m_slotListCrafting.getHeight();
		break;
	case eSectionClassicCraftingResult:
		sectionSize.x = m_slotListResult.getWidth();
		sectionSize.y = m_slotListResult.getHeight();
		break;
	default:
		assert(false);
		break;
	}

	int rows = getSectionRows(eSection);
	int cols = getSectionColumns(eSection);

	pSize->x = sectionSize.x / cols;
	pSize->y = sectionSize.y / rows;

	int itemCol = iItemIndex % cols;
	int itemRow = iItemIndex / cols;

	pPosition->x = itemCol * pSize->x;
	pPosition->y = itemRow * pSize->y;
}

void UIScene_ClassicCraftingMenu::setSectionSelectedSlot(ESceneSection eSection, int x, int y)
{
	int cols = getSectionColumns(eSection);

	int index = (y * cols) + x;

	UIControl_SlotList* slotList = nullptr;
	switch (eSection)
	{
	case eSectionClassicCraftingHotbar:
		slotList = &m_slotListHotbar;
		break;
	case eSectionClassicCraftingInventory:
		slotList = &m_slotListInventory;
		break;
	case eSectionClassicCraftingGrid:
		slotList = &m_slotListCrafting;
		break;
	case eSectionClassicCraftingResult:
		slotList = &m_slotListResult;
		break;
	default:
		assert(false);
		break;
	}

	slotList->setHighlightSlot(index);
}

UIControl* UIScene_ClassicCraftingMenu::getSection(ESceneSection eSection)
{
	UIControl* control = nullptr;
	switch (eSection)
	{
	case eSectionClassicCraftingHotbar:
		control = &m_slotListHotbar;
		break;
	case eSectionClassicCraftingInventory:
		control = &m_slotListInventory;
		break;
	case eSectionClassicCraftingGrid:
		control = &m_slotListCrafting;
		break;
	case eSectionClassicCraftingResult:
		control = &m_slotListResult;
		break;
	default:
		assert(false);
		break;
	}
	return control;
}
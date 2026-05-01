#pragma once

#include "UIScene_AbstractContainerMenu.h"
#include "IUIScene_ClassicCraftingMenu.h"

class InventoryMenu;

class UIScene_ClassicCraftingMenu : public UIScene_AbstractContainerMenu, public IUIScene_ClassicCraftingMenu
{
public:
	UIScene_ClassicCraftingMenu(int iPad, void* initData, UILayer* parentLayer);

	virtual EUIScene getSceneType() { return eUIScene_ClassicCraftingMenu; }

protected:
	UIControl_SlotList m_slotListCrafting, m_slotListResult;
	UIControl_Label m_labelTitle, m_labelInventory;

	UI_BEGIN_MAP_ELEMENTS_AND_NAMES(UIScene_AbstractContainerMenu)
		UI_BEGIN_MAP_CHILD_ELEMENTS(m_controlMainPanel)
		UI_MAP_ELEMENT(m_slotListCrafting, "CraftingList")
		UI_MAP_ELEMENT(m_slotListResult, "Result")
		UI_MAP_ELEMENT(m_labelTitle, "TitleLabel")
		UI_END_MAP_CHILD_ELEMENTS()
		UI_END_MAP_ELEMENTS_AND_NAMES()

		virtual wstring getMoviePath();
	virtual void handleReload();

	virtual int getSectionColumns(ESceneSection eSection);
	virtual int getSectionRows(ESceneSection eSection);
	virtual void GetPositionOfSection(ESceneSection eSection, UIVec2D* pPosition);
	virtual void GetItemScreenData(ESceneSection eSection, int iItemIndex, UIVec2D* pPosition, UIVec2D* pSize);
	virtual void handleSectionClick(ESceneSection eSection) {}
	virtual void setSectionSelectedSlot(ESceneSection eSection, int x, int y);

	virtual UIControl* getSection(ESceneSection eSection);
};
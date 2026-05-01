#pragma once

#include "UIScene_AbstractContainerMenu.h"
#include "IUIScene_InventoryMenu.h"

#include "..\..\..\Minecraft.World\MobEffect.h"

class InventoryMenu;

class UIScene_InventoryMenu : public UIScene_AbstractContainerMenu, public IUIScene_InventoryMenu
{
	friend class UIControl_MinecraftPlayer;
private:
	int m_bEffectTime[MobEffect::NUM_EFFECTS];
	int m_bIsClassicCrafting;

	void updateEffectsDisplay();
	void SetClassicCrafting(bool m_bIsClassicCrafting);

public:
	UIScene_InventoryMenu(int iPad, void* initData, UILayer* parentLayer);

	virtual EUIScene getSceneType() { return eUIScene_InventoryMenu; }

protected:
	UIControl_SlotList m_slotListArmor, m_slotListCrafting, m_slotListResult;
	UIControl_MinecraftPlayer m_playerPreview;
	UIControl_Label m_labelCrafting;

	IggyName m_funcSetClassicCrafting, m_funcUpdateEffects, m_funcAddEffect;

	UI_BEGIN_MAP_ELEMENTS_AND_NAMES(UIScene_AbstractContainerMenu)
		UI_BEGIN_MAP_CHILD_ELEMENTS(m_controlMainPanel)
		UI_MAP_ELEMENT(m_slotListArmor, "armorList")
		UI_MAP_ELEMENT(m_playerPreview, "iggy_player")
		UI_MAP_ELEMENT(m_slotListCrafting, "craftingList")
		UI_MAP_ELEMENT(m_slotListResult, "Result")
		UI_MAP_ELEMENT(m_labelCrafting, "craftingLabel")

		UI_MAP_NAME(m_funcSetClassicCrafting, L"SetClassicCrafting")
		UI_MAP_NAME(m_funcUpdateEffects, L"UpdateEffects")
		UI_MAP_NAME(m_funcAddEffect, L"AddEffect")
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

	virtual void customDraw(IggyCustomDrawCallbackRegion* region);
	virtual void handleTimerComplete(int id);
};
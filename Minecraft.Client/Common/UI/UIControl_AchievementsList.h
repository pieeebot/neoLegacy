#pragma once
#include "UIControl_Base.h"

class UIControl_AchievementsList : public UIControl_Base
{
private:
	IggyName m_funcAddNewItem, m_funcHighlightItem, m_funcSetTouchFocus, m_funcEnableButton;

public:
	UIControl_AchievementsList();
	void init(int id);
	int m_itemCount;
	int m_iCurrentSelection = 0;
	void SetTouchFocus(S32 iX, S32 iY, bool bRepeat);
	void UpdateControl();
	void EnableButton(int id, bool bEnable);
	void updateChildFocus(int iChild);
	void setCurrentSelection(int iSelection);
	virtual bool setupControl(UIScene* scene, IggyValuePath* parent, const string& controlName);
	void UIControl_AchievementsList::addnewItem(int id, wstring textureName);
	//virtual void ReInit();
};
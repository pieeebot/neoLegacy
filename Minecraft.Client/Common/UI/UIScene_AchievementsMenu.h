#pragma once
#include <string>

#include "Common/UI/UIScene.h"
#include "Common/UI/UIControl_Button.h"
#include "Common/UI/UIControl_Cursor.h"
#include "UIControl_AchievementsList.h"

class UIScene_AchievementsMenu : public UIScene
{
private:

	enum EControls
	{
		eControl_AchievementsLabel,
		eControl_AchievementsName,
		eControl_AchievementsDesc,
		eControl_AchievementsListContainer
	};
	UIControl m_controlMainPanel;
	UIControl_Label m_labelAchievements, m_labelName, m_labelDesc;
	UIControl_Cursor m_cursorPath1;
	vector<UIControl_Button*> m_AchButton;
	UIControl_AchievementsList m_achievementsList;
	IggyName m_funcSetDesc;
	
	UI_BEGIN_MAP_ELEMENTS_AND_NAMES(UIScene)
		UI_MAP_ELEMENT(m_labelAchievements, "AcheivementsLabel")
		UI_MAP_ELEMENT(m_labelName, "AchievementName")
		UI_MAP_ELEMENT(m_labelDesc, "AchievementDescription")
		//UI_MAP_ELEMENT(m_achievementsList, "AchievementsList")
		UI_MAP_ELEMENT(m_controlMainPanel, "AchievementsListContainer")
		UI_MAP_NAME(m_funcSetDesc, L"SetAchievementDescription")
		UI_BEGIN_MAP_CHILD_ELEMENTS(m_controlMainPanel)
			UI_MAP_ELEMENT(m_achievementsList, "AchievementsList")
		UI_END_MAP_CHILD_ELEMENTS()
	UI_END_MAP_ELEMENTS_AND_NAMES()

	bool showDescription = false;
	

public:
	struct SceneMousePos
	{
		F32 x;
		F32 y;
	};
	int deltaX;
	int deltaY;
	int aX;
	int aY;
	UIVec2D m_pointerPos;
	void getMouseToSWFScale(float& scaleX, float& scaleY);
	UIScene_AchievementsMenu(int iPad, void* initData, UILayer* parentLayer);
	SceneMousePos GetSceneMousePosition(UIScene* pScene, int rawMouseX, int rawMouseY);
	virtual void handleDestroy();
	int selection = 0;
	void SetAchievementDescription(wstring desc);
	virtual void tick();
	virtual EUIScene getSceneType() { return eUIScene_AchievementsMenu; }
	virtual void handleInput(int iPad, int key, bool repeat, bool pressed, bool released, bool& handled);
	virtual void handleFocusChange(F64 controlId, F64 childId);
	virtual void updateComponents();

	virtual void updateTooltips();
protected:
	virtual wstring getMoviePath();
}; 
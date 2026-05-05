#include "UIScene_AchievementsMenu.h"
#include "UI.h"
#include "UILayer.h"
#include "../Minecraft.World/HtmlString.h"
#include "../Minecraft.World/Achievements.h"
#include "../Minecraft.World/Achievement.h"
#include "UISplitScreenHelpers.h"
#include "StatsCounter.h"
#include "MultiPlayerLocalPlayer.h"
#include "Windows64/KeyboardMouseInput.h"

extern HWND g_hWnd;

UIScene_AchievementsMenu::UIScene_AchievementsMenu(int iPad, void* _initData, UILayer* parentLayer) : UIScene(iPad, parentLayer)
{
	// Setup all the Iggy references we need for this scene
	initialiseMovie();

	m_labelAchievements.init(L"Achievements");
	m_labelName.init(L"");
	m_achievementsList.init(0);
	for (int i = 0; i < Achievements::achievements->size(); i++) {
		std::wstring path;
		if (app.hasArchiveFile(L"Graphics\\TexturePackIcon.png"))
		{
			std::wstring iconStr(Achievements::achievements->at(i)->iconInt.begin(),
				Achievements::achievements->at(i)->iconInt.end());

			path = L"Graphics\\Achievements\\TROP" +
				(iconStr.empty() ? L"000" : iconStr) +
				L".png";
			byteArray ba = app.getArchiveFile(path);
			registerSubstitutionTexture(path, ba.data, ba.length);
		}


		std::string result = "Graphics\\Achievements\\" "TROP" + Achievements::achievements->at(i)->iconInt + ".png";
		
		m_achievementsList.addnewItem(i+1, path);

		int newPad = 0;
		if (Minecraft::GetInstance()->player != nullptr) {
			newPad = Minecraft::GetInstance()->player->GetXboxPad();
		}

		if (Minecraft::GetInstance()->stats[newPad]->hasTaken(Achievements::achievements->at(i)))
		{
			m_achievementsList.EnableButton(i, true);
		}
		else {
			m_achievementsList.EnableButton(i, false);
		}
	}
	m_funcSetDesc = registerFastName(L"SetAchievementDescription");

	m_achievementsList.setCurrentSelection(Minecraft::GetInstance()->achID + 1);
	m_achievementsList.m_iCurrentSelection = Minecraft::GetInstance()->achID + 1;

}

void UIScene_AchievementsMenu::handleDestroy()
{
	m_parentLayer->showComponent(m_iPad, eUIComponent_MenuBackground, false);
}

void UIScene_AchievementsMenu::SetAchievementDescription(wstring desc) {
	wstring desc2 = L"";

	if (desc != L"") {
		HtmlString test = HtmlString(desc, eHTMLColor_White);
		vector<HtmlString>* lines = new vector<HtmlString>();
		lines->push_back(test);
		desc2 = HtmlString::Compose(lines);
	}

	IggyDataValue result;
	IggyDataValue value[1];
	IggyStringUTF16 stringVal1;
	value[0].type = IGGY_DATATYPE_string_UTF16;
	stringVal1.string = (IggyUTF16*)desc2.c_str();
	stringVal1.length = desc2.length();
	value[0].string16 = stringVal1;

	IggyResult out = IggyPlayerCallMethodRS(getMovie(), &result, IggyPlayerRootPath(getMovie()), m_funcSetDesc, 1, value);
}

wstring UIScene_AchievementsMenu::getMoviePath()
{
	if (app.GetLocalPlayerCount() > 1)
	{
		return L"AchievementsMenu";
	}
	else
	{
		return L"AchievementsMenu";
	}
}

void UIScene_AchievementsMenu::updateComponents()
{
	bool bNotInGame = (Minecraft::GetInstance()->level == nullptr);
	if (!bNotInGame)
	{
		m_parentLayer->showComponent(m_iPad, eUIComponent_MenuBackground, true);
	}
	m_parentLayer->showComponent(m_iPad, eUIComponent_Logo, false);
}

void UIScene_AchievementsMenu::handleInput(int iPad, int key, bool repeat, bool pressed, bool released, bool& handled)
{
	ui.AnimateKeyPress(m_iPad, key, repeat, pressed, released);
	switch (key)
	{
	case ACTION_MENU_CANCEL:
		ui.NavigateBack(iPad);
		break;
	case ACTION_MENU_Y:
		if (pressed) {
			if (!g_KBMInput.IsKBMActive()) {
				showDescription = !showDescription;

				if (showDescription) {
					SetAchievementDescription(app.GetString(Achievements::achievements->at(m_achievementsList.m_iCurrentSelection - 1)->descID));
				}
				else {
					SetAchievementDescription(L"");
				}
			}
			sendInputToMovie(key, repeat, pressed, released);
			handled = true;
		}
		break;
	case ACTION_MENU_X:
		if (pressed) {
			if (g_KBMInput.IsKBMActive()) {
				showDescription = !showDescription;

				if (showDescription) {
					SetAchievementDescription(app.GetString(Achievements::achievements->at(m_achievementsList.m_iCurrentSelection - 1)->descID));
				}
				else {
					SetAchievementDescription(L"");
				}
			}
			sendInputToMovie(key, repeat, pressed, released);
			handled = true;
		}
		break;
	case ACTION_MENU_UP:
		if (pressed) {
			if (m_achievementsList.m_iCurrentSelection > 10) {
				m_achievementsList.m_iCurrentSelection -= 10;
			}
		}
		sendInputToMovie(key, repeat, pressed, released);
		handled = true;
		break;
	case ACTION_MENU_DOWN:
		if (pressed) {
			if (m_achievementsList.m_iCurrentSelection < Achievements::achievements->size() - 10) {
				m_achievementsList.m_iCurrentSelection += 10;
			}
		}
		sendInputToMovie(key, repeat, pressed, released);
		handled = true;
		break;
	case ACTION_MENU_LEFT:
		if (pressed) {
			if (m_achievementsList.m_iCurrentSelection > 1) {
				m_achievementsList.m_iCurrentSelection -= 1;
			}
		}
		sendInputToMovie(key, repeat, pressed, released);
		handled = true;
		break;
	case ACTION_MENU_RIGHT:
		if (pressed) {
			if (m_achievementsList.m_iCurrentSelection < Achievements::achievements->size()) {
				m_achievementsList.m_iCurrentSelection += 1;
			}
		}
		sendInputToMovie(key, repeat, pressed, released);
		handled = true;
		break;
	}
};

void UIScene_AchievementsMenu::getMouseToSWFScale(float& scaleX, float& scaleY)
{
	extern HWND g_hWnd;
	RECT rc;
	GetClientRect(g_hWnd, &rc);
	int winW = rc.right - rc.left;
	int winH = rc.bottom - rc.top;
	if (winW <= 0 || winH <= 0) { scaleX = 1.0f; scaleY = 1.0f; return; }

	S32 renderW, renderH;
	C4JRender::eViewportType vp = GetParentLayer()->getViewport();
	ui.getRenderDimensions(vp, renderW, renderH);
	if (vp != C4JRender::VIEWPORT_TYPE_FULLSCREEN)
		Fit16x9(renderW, renderH);

	float screenW = (float)ui.getScreenWidth();
	float screenH = (float)ui.getScreenHeight();
	scaleX = static_cast<float>(m_movieWidth) * screenW / (static_cast<float>(renderW) * static_cast<float>(winW));
	scaleY = static_cast<float>(m_movieHeight) * screenH / (static_cast<float>(renderH) * static_cast<float>(winH));
}

void UIScene_AchievementsMenu::tick()
{
	UIScene::tick();
	

	if (m_achievementsList.m_iCurrentSelection != selection && m_achievementsList.m_iCurrentSelection != 0) {
		m_achievementsList.setCurrentSelection(m_achievementsList.m_iCurrentSelection - 1);
		m_labelName.setLabel(app.GetString(Achievements::achievements->at(m_achievementsList.m_iCurrentSelection - 1)->nameID));
		if (showDescription) {
			SetAchievementDescription(app.GetString(Achievements::achievements->at(m_achievementsList.m_iCurrentSelection - 1)->descID));
		}
		else {
			SetAchievementDescription(L"");
		}
	}
	selection = m_achievementsList.m_iCurrentSelection;
}

void UIScene_AchievementsMenu::updateTooltips()
{
	ui.SetTooltips(m_iPad, -1, IDS_TOOLTIPS_CANCEL, -1, IDS_TOOLTIPS_SHOW_DESCRIPTION);
}

void UIScene_AchievementsMenu::handleFocusChange(F64 controlId, F64 childId)
{
	switch (static_cast<int>(controlId))
	{
	case 0:
		m_achievementsList.updateChildFocus(static_cast<int>(childId));
		break;
	};
	updateTooltips();
}
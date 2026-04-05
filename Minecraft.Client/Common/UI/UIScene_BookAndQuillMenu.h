#pragma once
#include <string>

#include "Common/UI/UIScene.h"
#include "Common/UI/UIControl_Book.h"
#include "Common/UI/UIControl_PageFlip.h"
#include "Common/UI/IUIScene_AbstractContainerMenu.h"
#include "IUIScene_WritingBookMenu.h"

class UIScene_BookAndQuillMenu : public UIScene, public IUIScene_WritingBookMenu
{
private:
	enum eControls
	{
		eControl_Sign,
		eControl_Done,
		eControl_Page,
		eControl_Type,
		eControl_Caret,
		eControl_Book,
		eControl_1,
		eControl_2,
	};

	UIControl m_controlBackground;
	UIControl_Book m_Book;
	UIControl_PageFlip arro1, arro2;

	UIControl_Button m_buttonSign, m_buttonDone;
	UIControl_Label m_pageText, m_typeText, m_caretText;
	UIControl_TextInput m_test;
	IggyName m_funcSetCaretIndex;
	IggyName m_funcSetBookIsSigning;
	IggyName m_funcChangePage;
	IggyName m_funcUpdateKBM;
	IggyName m_funcUpdatePageVisibility;
	IggyName m_funcSet;
	IggyName m_funcSetLocalLabel;
	IggyName m_funcSign;
	IggyName m_funcGetLabel;
	UI_BEGIN_MAP_ELEMENTS_AND_NAMES(UIScene)
		UI_MAP_ELEMENT(m_buttonSign, "ButtonSign")
		UI_MAP_ELEMENT(m_buttonDone, "ButtonClose")
		UI_MAP_ELEMENT(m_pageText, "PageLabel")
		UI_MAP_ELEMENT(m_caretText, "Caret")
		UI_MAP_ELEMENT(m_Book, "Book")
		UI_MAP_ELEMENT(m_typeText, "BookAndQuillLabel")
		UI_MAP_ELEMENT(arro1, "BookArrowLeft")
		UI_MAP_ELEMENT(arro2, "BookArrowRight")
		UI_MAP_NAME(m_funcSign, L"SetBookIsSigned")
		UI_MAP_NAME(m_funcGetLabel, L"gHeight")
		UI_MAP_NAME(m_funcSetLocalLabel, L"setLocalLabel")
		UI_MAP_NAME(m_funcUpdateKBM, L"m_OnKBM")
		//UI_MAP_ELEMENT(m_test, "Text")
	UI_END_MAP_ELEMENTS_AND_NAMES()

public:
	//std::shared_ptr<ItemInstance> m_itemInstance;
	ListTag<Tag> pagesTagOG;
	CompoundTag* itemTag;
	int currentPage = 1;
	//int totalPages = 1;
	int cachedID = -1;
	int heldItem;
	bool KBM;
	bool wasInMessage = false;
	Player* player;
	std::wstring localUsername;

	WritingBookMenuParams* data;

	UIScene_BookAndQuillMenu(int iPad, void* initData, UILayer* parentLayer);

	using UIScene::reloadMovie;
	virtual void reloadMovie(bool force);
	virtual bool needsReloaded();

	virtual EUIScene getSceneType() { return eUIScene_BookMenu; }

	static int KeyboardCompleteCallback(LPVOID lpParam, bool bRes);

	virtual void handleInput(int iPad, int key, bool repeat, bool pressed, bool released, bool &handled);	

	virtual void KBMUpdate(bool bVal);

	//virtual void updateTooltips();
	void UpdateCaretIndex(int index);
	void ChangePage(bool arrow = true, bool updateBoth = true);
	void IsKBMActive(bool bVal);
	void SetBookIsSigning(bool bIsSigning);
	void SetLocalLabel(wstring m_label1);
	void Update();

	static int WarningExitBookReturned(void* pParam, int iPad, C4JStorage::EMessageResult result);

	C4JStorage::EMessageResult result1;

	virtual void tick();

	virtual void getDirectEditLabels(vector<UIControl_Label*> &labels);

	void UIScene_BookAndQuillMenu::SetPageText(wstring text, UIControl_Label label);
protected:
	virtual wstring getMoviePath();
	virtual void handlePress(F64 controlId, F64 childId) override;
};

#include "stdafx.h"
#include "Common\UI\UI.h"
#include "UIScene_BookAndQuillMenu.h"
#include "PlayerConnection.h"
#include "LocalPlayer.h"
#include "ClientConnection.h"
#include "..\Minecraft.Client\PlayerList.h"
#include "..\Minecraft.Client\ServerPlayer.h"
#include "..\Minecraft.Client\MultiplayerLocalPlayer.h"

UIScene_BookAndQuillMenu::UIScene_BookAndQuillMenu(int iPad, void *initData, UILayer *parentLayer) : UIScene(iPad, parentLayer)
{
	iPadLocal = iPad;
	// Setup all the Iggy references we need for this scene
	g_KBMInput.ClearCharBuffer();
	data = static_cast<WritingBookMenuParams*>(initData);
	initialiseMovie();
	m_itemInstance = data->itemInstance;

	itemTag = m_itemInstance->getTag();
	if (itemTag != nullptr) {
		if (itemTag->contains(L"pages"))
		{
			pagesTagOG = *static_cast<ListTag<Tag>*>(itemTag->getList(L"pages")->copy());
		}
	}
	else {
		pagesTagOG = *new ListTag<Tag>(L"pages");

		CompoundTag* firstPageTag = new CompoundTag(L"");
		firstPageTag->putString(L"text", L"");

		pagesTagOG.add(firstPageTag);
	}
	
	//Set the book as signed in the SWF
	if (m_itemInstance->id == 387) {
		IggyDataValue result;
		IggyResult out = IggyPlayerCallMethodRS(this->getMovie(), &result, IggyPlayerRootPath(getMovie()), m_funcSign, 0, nullptr);
		
		signedBook = true;
	}

	m_funcSetCaretIndex = registerFastName(L"SetCaretPosition");
	m_funcSetBookIsSigning = registerFastName(L"SetBookIsSigning");
	m_funcChangePage = registerFastName(L"ChangePage");
	m_funcUpdatePageVisibility = registerFastName(L"UpdatePageVisibility");
	m_funcSet = registerFastName(L"Update");

	app.DebugPrintf(std::to_string(iRealWidth).c_str());


	Init(m_itemInstance);
	player = data->player.get();

	heldItem = 4;


	m_buttonSign.init(L"Sign", eControl_Sign);
	m_buttonDone.init(L"Done", eControl_Done);
	m_pageText.init(L"Page 1 of ???", eControl_Page);
	m_caretText.init(L"", eControl_Caret);
	m_Book.init(L"", eControl_Book);
	arro1.init(L"", eControl_1);
	arro2.init(L"", eControl_2);
	m_typeText.init(L"", eControl_Type);
	ChangePage(false, true);


	IsKBMActive(g_KBMInput.IsKBMActive());
	KBM = g_KBMInput.IsKBMActive();
	

	SetPageText(getCurrentPageText(), m_typeText);
	m_typeText.m_iCursorPos = m_typeText.m_editBuffer.length();
	UpdateCaretIndex(m_typeText.m_editBuffer.length());
	m_pageText.setLabel(std::wstring(L"Page ") + L"1" + L" of " + std::to_wstring(m_totalPages));


	

#ifdef _WINDOWS64
		if (signedBook == true) {
			return;
		}
		if (!g_KBMInput.IsKBMActive())
		{
			//UIKeyboardInitData kbData;
			//kbData.title = L"Enter Book Text";//app.GetString(IDS_CREATE_NEW_WORLD); <- for future reference when using languages.loc
			//kbData.defaultText = m_typeText.m_editBuffer.c_str();
			//kbData.maxChars    = 1023;
			//kbData.callback    = &KeyboardCompleteCallback;
			//kbData.lpParam     = this;
			//ui.NavigateToScene(m_iPad, eUIScene_Keyboard, &kbData, eUILayer_Fullscreen, eUIGroup_Fullscreen);
		}
		else if (g_KBMInput.IsKBMActive())
		{
			m_typeText.beginDirectEdit(1023, false, L"");
			app.DebugPrintf("starting direct edit\n");
			
		}
		else {
			//Make it so that keyboard buffer is cleared, as to not have text come back if it shouldn't exist
			g_KBMInput.ClearCharBuffer();
		}
#endif

		this->SetFocusToElement(eControl_Type);
		m_typeText.m_bhasBeenSelected = true;

}

void UIScene_BookAndQuillMenu::SetPageText(wstring text, UIControl_Label label) {
	m_typeText.setLabel(text);
	m_typeText.m_editBuffer = text;
	m_typeText.m_iCursorPos = m_typeText.m_editBuffer.length();
}


void UIScene_BookAndQuillMenu::tick() 
{
	UIScene::tick();
}

void UIScene_BookAndQuillMenu::KBMUpdate(bool bVal) {
	if (bVal == KBM) {
		return;
	}
	else {
		KBM = bVal;
	}
	IsKBMActive(bVal);

	if (bVal == true) {
		m_typeText.beginDirectEdit(1023, false, L"");

		this->SetFocusToElement(eControl_Type);
		m_typeText.m_bhasBeenSelected = true;
		UpdateCaretIndex(0);
	}
}

#ifdef _WINDOWS64
void UIScene_BookAndQuillMenu::getDirectEditLabels(vector<UIControl_Label*> &labels) {
    labels.push_back(&m_typeText);
}
#endif

wstring UIScene_BookAndQuillMenu::getMoviePath()
{
	return L"BookAndQuillMenu";
}

void UIScene_BookAndQuillMenu::reloadMovie(bool force)
{
	// Never needs reloaded
}

bool UIScene_BookAndQuillMenu::needsReloaded()
{
	// Never needs reloaded
	return false;
}

//Bunch of Iggy BS
void UIScene_BookAndQuillMenu::UpdateCaretIndex(int index) {
	
	IggyDataValue result;
	IggyDataValue value[2];
	value[0].type = IGGY_DATATYPE_number;
	if (!signing) {
		value[0].number = m_typeText.m_iCursorPos;
	}
	else {
		value[0].number = m_typeText.m_iCursorPos + 18;
	}
	value[1].type = IGGY_DATATYPE_boolean;
	value[1].boolval = (m_typeText.m_iCursorPos == m_typeText.m_editBuffer.length());
	IggyResult out = IggyPlayerCallMethodRS(getMovie(), &result, m_rootPath, m_funcSetCaretIndex, 2, value);
}

//arrow false updates left arrow, arrow true updates right arrow
void UIScene_BookAndQuillMenu::ChangePage(bool arrow, bool updateBoth) {
	IggyDataValue result;
	IggyDataValue value[4];
	value[0].type = IGGY_DATATYPE_number;
	value[0].number = m_currentPageIndex;
	value[1].type = IGGY_DATATYPE_number;
	value[1].number = m_totalPages;
	value[2].type = IGGY_DATATYPE_boolean;
	value[2].boolval = arrow;
	value[3].type = IGGY_DATATYPE_boolean;
	value[3].boolval = updateBoth;
	IggyResult out = IggyPlayerCallMethodRS(getMovie(), &result, m_rootPath, m_funcChangePage, 4, value);
}

void UIScene_BookAndQuillMenu::IsKBMActive(bool bVal) {

	//RADEXPFUNC IggyResult RADEXPLINK IggyValueGetBooleanRS(IggyValuePath *var, IggyName sub_name, char const *sub_name_utf8, rrbool *result);

	/*IggyDataValue result;
	IggyDataValue value[1];
	value[0].type = IGGY_DATATYPE_boolean;
	value[0].boolval = bVal;
	IggyResult out = IggyPlayerCallMethodRS(getMovie(), &result, m_rootPath, m_funcChangePage, 1, value);*/

	rrbool newBool = bVal;

	rrbool out = IggyValueSetBooleanRS(m_rootPath, m_funcUpdateKBM, nullptr, newBool);
}

void UIScene_BookAndQuillMenu::SetBookIsSigning(bool isSigning) {
	IggyDataValue result;
	IggyDataValue value[1];
	value[0].type = IGGY_DATATYPE_boolean;
	value[0].boolval = isSigning;
	IggyResult out = IggyPlayerCallMethodRS(getMovie(), &result, m_rootPath, m_funcSetBookIsSigning, 1, value);
}

void UIScene_BookAndQuillMenu::SetLocalLabel(wstring m_label1) {
	IggyDataValue result;
	IggyDataValue value[1];
	value[0].type = IGGY_DATATYPE_string_UTF16;
	IggyStringUTF16 stringVal;

	stringVal.string = (IggyUTF16*)m_label1.c_str();
	stringVal.length = m_label1.length();
	value[0].string16 = stringVal;

	IggyResult out = IggyPlayerCallMethodRS(getMovie(), &result, m_rootPath, m_funcSetLocalLabel, 1, value);
}

void UIScene_BookAndQuillMenu::Update() {
	IggyDataValue result;
	IggyResult out = IggyPlayerCallMethodRS(getMovie(), &result, m_rootPath, m_funcSet, 0, nullptr);
}

int UIScene_BookAndQuillMenu::WarningExitBookReturned(void* pParam, int iPad, C4JStorage::EMessageResult result)
{
	UIScene_BookAndQuillMenu* pClass = static_cast<UIScene_BookAndQuillMenu*>(pParam);

	if (result == C4JStorage::EMessage_ResultAccept) {
		//Reset the book to the previous state, should in theory do so already but it seems changes
		//directly affect our itemInstance, but the changes are just ghost changes (changing item slots reverts it)
		for (int i = 0; i <= pClass->pagesTagOG.size()-1; i++) {
			auto t = static_cast<CompoundTag*>(pClass->pagesTagOG.get(i));
			pClass->setCurrentPageText(t->getString(L"text"), i);
		}
		if (pClass->itemTag != nullptr) pClass->itemTag->getList(L"pages")->getList().resize(pClass->pagesTagOG.size());


		pClass->navigateBack();
		pClass->m_typeText.m_bhasBeenSelected = false;
		return 0;
	}
	else {
		return -1;
	}
}

void UIScene_BookAndQuillMenu::handleInput(int iPad, int key, bool repeat, bool pressed, bool released, bool &handled)
{
	auto item = Minecraft::GetInstance()->localplayers[iPadLocal]->inventory->getSelected();
	ListTag<Tag>* pagesTag = new ListTag<Tag>();
	ui.AnimateKeyPress(m_iPad, key, repeat, pressed, released);
	switch(key)
	{
	case ACTION_MENU_CANCEL:
		if(pressed)
		{
			if (!signing)
			{
				CompoundTag* itemTag = m_itemInstance->getTag();
				setCurrentPageText(m_typeText.m_editBuffer);
				if (signedBook == false) {
					if (!pagesTagOG.equals(m_currentPage)) {
						UINT uiIDA[2];
						uiIDA[0] = IDS_CONFIRM_OK;
						uiIDA[1] = IDS_CONFIRM_CANCEL;
						int r = ui.RequestAlertMessage(IDS_TITLE_EXITBOOK, IDS_DESC_EXITBOOK, uiIDA, 2, m_iPad, &UIScene_BookAndQuillMenu::WarningExitBookReturned, this);
						if (r == 0)
						{
							navigateBack();
						}
					}
					else {
						navigateBack();
					}
				}
			}
			else
			{
				SetPageText(getCurrentPageText(), m_typeText);
				m_typeText.signing = false;
				m_buttonSign.setLabel(L"Sign");
				m_buttonDone.setLabel(L"Done");
				m_pageText.setVisible(true);
				SetBookIsSigning(false);
				signing = false;

				this->SetFocusToElement(eControl_Type);
				m_typeText.beginDirectEdit(1023, false, L"");

				break;
				
			}
			if (signedBook) {
				navigateBack();
			}
			//m_typeText.m_bhasBeenSelected = false;
			
			//cachedID = eControl_Done;
			//m_typeText.confirmDirectEdit();
			break;
		}
		break;
	case ACTION_MENU_OK:
	case ACTION_MENU_UP:
	case ACTION_MENU_DOWN:
	case ACTION_MENU_LEFT:
	case ACTION_MENU_RIGHT:
		sendInputToMovie(key, repeat, pressed, released);
		handled = true;
		break;
	case ACTION_MENU_PAGEDOWN:
		//If on KBM, use PGUP/PGDWN instead, as SCROLL conflicts with Q and E
		if (g_KBMInput.IsKBMActive()) {
			if (pressed && !repeat)
			{
				setCurrentPageText(m_typeText.m_editBuffer);
				ScrollPageRight();
				SetPageText(getCurrentPageText(), m_typeText);
				m_pageText.setLabel(L"Page " + to_wstring(m_currentPageIndex+1) + L" of " + to_wstring(m_totalPages));
				ChangePage();
				handled = true;
			}
			sendInputToMovie(key, repeat, pressed, released);
			break;
		}
		break;
	case ACTION_MENU_PAGEUP:
		if (g_KBMInput.IsKBMActive()) {
			if (pressed && !repeat)
			{
				setCurrentPageText(m_typeText.m_editBuffer);
				ScrollPageLeft();
				SetPageText(getCurrentPageText(), m_typeText);
				m_pageText.setLabel(L"Page " + to_wstring(m_currentPageIndex+1) + L" of " + to_wstring(m_totalPages));
				ChangePage();
				handled = true;
			}
			sendInputToMovie(key, repeat, pressed, released);
			break;
		}
		break;
	case ACTION_MENU_RIGHT_SCROLL:
		//Regular keybinds
		if (!g_KBMInput.IsKBMActive()) {
			if (pressed && !repeat)
			{
				setCurrentPageText(m_typeText.m_editBuffer);
				ScrollPageRight();
				SetPageText(getCurrentPageText(), m_typeText);
				m_pageText.setLabel(L"Page " + to_wstring(m_currentPageIndex+1) + L" of " + to_wstring(m_totalPages));
				ChangePage();
				handled = true;
				sendInputToMovie(key, repeat, pressed, released);
			}
			break;
		}
		break;
	case ACTION_MENU_LEFT_SCROLL:
		//Regular keybinds
		if (!g_KBMInput.IsKBMActive()) {
			if (pressed && !repeat)
			{
				setCurrentPageText(m_typeText.m_editBuffer);
				ScrollPageLeft();
				SetPageText(getCurrentPageText(), m_typeText);
				m_pageText.setLabel(L"Page " + to_wstring(m_currentPageIndex+1) + L" of " + to_wstring(m_totalPages));
				ChangePage();
				handled = true;
				sendInputToMovie(key, repeat, pressed, released);
			}
			break;
		}
		break;
	}
	IUIScene_WritingBookMenu::updateTooltips();
}

int UIScene_BookAndQuillMenu::KeyboardCompleteCallback(LPVOID lpParam, bool bRes)
{
	if (bRes)
    {
		uint16_t pchText[128];
		ZeroMemory(pchText, 128 * sizeof(uint16_t));
#ifdef _WINDOWS64
		Win64_GetKeyboardText(pchText, 128);
#else
		InputManager.GetText(pchText);
#endif
		UIScene_BookAndQuillMenu* pClass = static_cast<UIScene_BookAndQuillMenu*>(lpParam);

		if (pchText[0] != 0)
		{
			pClass->m_typeText.setLabel((wchar_t*)pchText);
			pClass->m_typeText.m_editBuffer = (wchar_t*)pchText;
			pClass->m_typeText.m_iCursorPos = wcslen((wchar_t*)pchText);
			pClass->UpdateCaretIndex(0);
		}
	}
	return 0;
}



void UIScene_BookAndQuillMenu::handlePress(F64 controlId, F64 childId)
{
	//Packet
	ByteArrayOutputStream baos;
	DataOutputStream dos(&baos);

	//Mouse centering
	S32 width, height;
	m_parentLayer->getRenderDimensions(width, height);
	S32 x = (static_cast<float>(width) / m_movieWidth);
	S32 y = (static_cast<float>(height) / m_movieHeight);
	HWND hwnd = GetForegroundWindow();
	WINDOWPLACEMENT wp{};


	switch((int)controlId)
	{
	case eControl_Type:
#ifdef _WINDOWS64
		if (signedBook == true) {
			break;
		}

		if (!InputManager.ButtonPressed(iPadLocal, ACTION_MENU_OK) && !g_KBMInput.IsKBMActive()) {
			break;
		}
		if (!g_KBMInput.IsKBMActive())
		{
			UIKeyboardInitData kbData;
			kbData.title = L"Enter Book Text";//app.GetString(IDS_CREATE_NEW_WORLD);
			kbData.defaultText = m_typeText.m_editBuffer.c_str();
			kbData.maxChars    = 1023;
			kbData.callback    = &KeyboardCompleteCallback;
			kbData.lpParam     = this;
			ui.NavigateToScene(m_iPad, eUIScene_Keyboard, &kbData, eUILayer_Fullscreen, eUIGroup_Fullscreen);
		}
		else if (m_typeText.hasFocus() == true)
		{
				//m_bIgnoreInput = false;
			if (cachedID != controlId) {
				m_typeText.beginDirectEdit(1023, false, L"");
				app.DebugPrintf("starting direct edit\n");

				cachedID = controlId;
			}
			
		}
		else {
			//Make it so that keyboard buffer is cleared, as to not have text come back if it shouldn't exist
			g_KBMInput.ClearCharBuffer();
		}
#else
		InputManager.RequestKeyboard(L"Message", L"", 0, 256, &KeyboardCompleteCallback,this,C_4JInput::EKeyboardMode_Alphabet);
#endif
		break;
	case eControl_Book:
		if (g_KBMInput.IsKBMActive()) {
			//This does not work when using controller. Why? God knows...
			this->SetFocusToElement(eControl_Type);
			m_typeText.beginDirectEdit(1023, false, L"");
		}
		else {
			if (signedBook == true) {
				break;
			}
			if (!InputManager.ButtonPressed(iPadLocal, ACTION_MENU_OK) && !g_KBMInput.IsKBMActive()) {
				break;
			}
			
			UIKeyboardInitData kbData;
			kbData.title = L"Enter Book Text";//app.GetString(IDS_CREATE_NEW_WORLD);
			kbData.defaultText = m_typeText.m_editBuffer.c_str();
			kbData.maxChars = 1023;
			kbData.callback = &KeyboardCompleteCallback;
			kbData.lpParam = this;
			ui.NavigateToScene(m_iPad, eUIScene_Keyboard, &kbData, eUILayer_Fullscreen, eUIGroup_Fullscreen);
		}
		
		break;
	//Left and Right Arrows
	case eControl_1:
		setCurrentPageText(m_typeText.m_editBuffer);
		ScrollPageLeft();
		SetPageText(getCurrentPageText(), m_typeText);
		m_pageText.setLabel(L"Page " + to_wstring(m_currentPageIndex + 1) + L" of " + to_wstring(m_totalPages));
		ChangePage(true, false);
		break;
	case eControl_2:
		setCurrentPageText(m_typeText.m_editBuffer);
		ScrollPageRight();
		SetPageText(getCurrentPageText(), m_typeText);
		m_pageText.setLabel(L"Page " + to_wstring(m_currentPageIndex + 1) + L" of " + to_wstring(m_totalPages));
		ChangePage(false, false);
		break;
	case eControl_Done:
		if (!signing)
		{
			setCurrentPageText(m_typeText.m_editBuffer);
			SaveChanges(false);
			navigateBack();
		}
		else
		{
			SetPageText(getCurrentPageText(), m_typeText);
			m_typeText.signing = false;
			m_buttonSign.setLabel(L"Sign");
			m_buttonDone.setLabel(L"Done");
			m_pageText.setVisible(true);
			SetBookIsSigning(false);
			signing = false;
		}

		this->SetFocusToElement(eControl_Type);
		m_typeText.m_bhasBeenSelected = false;
		cachedID = eControl_Done;
		break;
	case eControl_Sign:
			S32 width, height;
			m_parentLayer->getRenderDimensions(width, height);
			S32 x = (static_cast<float>(width) / m_movieWidth);
			S32 y = (static_cast<float>(height) / m_movieHeight);
			HWND hwnd = GetForegroundWindow();
			WINDOWPLACEMENT wp{};

			if (!signing) {

				setCurrentPageText(m_typeText.m_editBuffer);
				//SaveChanges(false);
				m_typeText.m_editBuffer = L"";
				m_typeText.signing = true;
				m_buttonSign.setLabel(L"Sign and Close");
				m_buttonDone.setLabel(L"Cancel");
				m_pageText.setVisible(false);
				SetBookIsSigning(true);
				signing = true;
			}
			else {
				m_bookTitle = m_typeText.m_editBuffer;
				if (m_bookTitle.length() != 0) {
					SaveChanges(true);
					navigateBack();
				}
			}
			m_typeText.beginDirectEdit(15, true, data->player->getDisplayName());

			this->SetFocusToElement(eControl_Type);
			m_typeText.m_bhasBeenSelected = true;

			//Center mouse position
			wp.length = sizeof(WINDOWPLACEMENT);
			GetWindowPlacement(hwnd, &wp);
			SetCursorPos(wp.rcNormalPosition.left + width / 2, wp.rcNormalPosition.top + height / 2);
			cachedID = eControl_Type;
	};
}
#include "stdafx.h"
#include "UI.h"
#include "UIControl_Label.h"
#include "..\..\..\Minecraft.World\StringHelpers.h"

UIControl_Label::UIControl_Label()
{
	m_reinitEnabled = true;
}

bool UIControl_Label::setupControl(UIScene *scene, IggyValuePath *parent, const string &controlName)
{
	if (!parent) {
		
	}
	UIControl::setControlType(UIControl::eLabel);
	bool success = UIControl_Base::setupControl(scene,parent,controlName);

	//Label specific initialisers
    //m_funcGetLabel = registerFastName(L"gHeight");
    m_funcSetCaretIndex = registerFastName(L"SetCaretIndex");
	return success;
}

void UIControl_Label::UpdateCaretIndex(int index) {
    if (!m_parentScene || !m_parentScene->getMovie())
        return;

    IggyDataValue result;
    IggyDataValue value[1];
    value[0].type = IGGY_DATATYPE_number;
    value[0].number = index;
    IggyResult out = IggyPlayerCallMethodRS(m_parentScene->getMovie(), &result, getIggyValuePath(), m_funcSetCaretIndex, 1, value);
}

void UIControl_Label::init(UIString label, int id)
{
	m_label = label;
	m_id = id;

	IggyDataValue result;
	IggyDataValue value[2];
	value[0].type = IGGY_DATATYPE_string_UTF16;
	IggyStringUTF16 stringVal;

	stringVal.string = (IggyUTF16*)label.c_str();
	stringVal.length = label.length();
	value[0].string16 = stringVal;

	value[1].type = IGGY_DATATYPE_number;
	value[1].number = id;

	app.DebugPrintf(("Initialised with id" + std::to_string(id)).c_str());
	IggyResult out = IggyPlayerCallMethodRS ( m_parentScene->getMovie() , &result, getIggyValuePath() , m_initFunc , 2 , value );

	app.DebugPrintf(("Initialised with result " + std::to_string(out)).c_str());
}

void UIControl_Label::ReInit()
{
	UIControl_Base::ReInit();
	
	// 4J-JEV: This can't be reinitialised.
	/*if (m_reinitEnabled)
	{
		init(m_label);
	}*/

	init(m_label, m_id);
}

#ifdef _WINDOWS64

void UIControl_Label::beginDirectEdit(int charLimit, bool bSigning, wstring author)
{
	m_textBeforeEdit = m_editBuffer;
	m_iCharLimit = charLimit;
	m_bDirectEditing = true;
	m_iDirectEditCooldown = 0;
	m_iCaretBlinkTimer = 0;
    authorName = author;

    signing = bSigning;

	g_KBMInput.ClearCharBuffer();
}

UIControl_Label::EDirectEditResult UIControl_Label::tickDirectEdit()
{
    if (this->hasFocus() == false) {
        //Clear characters typed while not in focus, or we'll get a suprise... ;3
        g_KBMInput.ClearCharBuffer();
		return eDirectEdit_Continue;
    }

    auto parent = (UIScene_BookAndQuillMenu*)this->getParentScene();
    
    if (m_iDirectEditCooldown > 0)
        m_iDirectEditCooldown--;

    if (!m_bDirectEditing)
    {
        return eDirectEdit_Continue;
    }
    int bufferLen;
    if (!signing) {
        bufferLen = (int)m_editBuffer.length();
    }
    else {
		bufferLen = (int)m_editBuffer.length();
    }
    

    // Clamp cursor position to valid range
    if (m_iCursorPos < m_cPosMin) m_iCursorPos = m_cPosMin;
    if (m_iCursorPos > bufferLen) m_iCursorPos = bufferLen;

    wstring display;
    // Display cursor
    if (!signing) {
        display = m_editBuffer;
    }
    else {
        display = L"Enter Book Title:\r" + m_editBuffer + L"   \rby " + authorName + L"\r\rNote! When you sign the book, it will no longer be editable.";
    }
    
    auto countLines = [](const wstring& text) -> int {
        int total = 0;
        size_t start = 0;
        while (true) {
            size_t newline = text.find(L'\r', start);
            size_t segLen = (newline == wstring::npos) ? text.length() - start : newline - start;
            total += max(1, (int)((segLen + 22) / 23));
            if (newline == wstring::npos) break;
            start = newline + 1;
        }
        return total;
    };

    EDirectEditResult result = eDirectEdit_Continue;
    bool changed = false;
    
    // Consume typed characters
    wchar_t ch;
    while (g_KBMInput.ConsumeChar(ch))
    {
        iRealWidth = height();
        app.DebugPrintf(std::to_string(iRealWidth).c_str());

        if (ch == 0x08) // Backspace
        {
            if (m_iCursorPos > 0 && m_iCursorPos <= (int)m_editBuffer.length())
            {
                m_editBuffer.erase(m_iCursorPos - 1, 1);
                m_iCursorPos--;
                changed = true;

            }
        }
        else if (ch == 0x0D) // Enter
        {
            if (iRealWidth + 32 < 480) {
                m_editBuffer.insert(m_iCursorPos, L"\r");
                m_iCursorPos++;
                changed = true;
            }
            
        }
        else if (m_iCharLimit <= 0 || (int)m_editBuffer.length() < m_iCharLimit)
        {
            if (m_iCursorPos >= 0 && m_iCursorPos <= (int)m_editBuffer.length())
            {
                if (iRealWidth + 32 < 480) {
                    m_editBuffer.insert(m_iCursorPos, 1, ch);
                    m_iCursorPos++;
                    changed = true;
                }
            }
        }
    }
    // Paste from clipboard
    if (g_KBMInput.IsKeyPressed('V') && g_KBMInput.IsKeyDown(VK_CONTROL))
    {
        wstring pasted = Screen::getClipboard();
        wstring sanitized;
        sanitized.reserve(pasted.length());

        for (wchar_t pc : pasted)
        {
            if (pc >= 0x20) // Keep printable characters
            {
                if (m_iCharLimit > 0 && (m_editBuffer.length() + sanitized.length()) >= (size_t)m_iCharLimit)
                    break;
                sanitized += pc;
            }
        }

        if (!sanitized.empty())
        {
            m_editBuffer.insert(m_iCursorPos, sanitized);
            m_iCursorPos += (int)sanitized.length();
            changed = true;
        }
    }

    setLabel(display);
    if (!signing) {
        parent->SetLocalLabel(m_editBuffer);
    }
    else {
        parent->SetLocalLabel(display);
    }
    
    parent->UpdateCaretIndex(m_iCursorPos);

    bufferLen = (int)m_editBuffer.length();
    if (g_KBMInput.IsKeyPressed(VK_UP || VK_DOWN)) 
    {
        return eDirectEdit_Continue;
    }
    if (m_iCursorPos > bufferLen) {
        m_iCursorPos = bufferLen;
    }

    // Arrow keys
    if (g_KBMInput.IsKeyPressed(VK_LEFT) && m_iCursorPos > 0)
    {
        m_iCursorPos--;
    }
    if (g_KBMInput.IsKeyPressed(VK_RIGHT) && m_iCursorPos < bufferLen)
    {
        m_iCursorPos++;
    }
    if (g_KBMInput.IsKeyPressed(VK_HOME))
    {
        m_iCursorPos = 0;
    }
    if (g_KBMInput.IsKeyPressed(VK_END))
    {
        m_iCursorPos = bufferLen;
    }
    if (g_KBMInput.IsKeyPressed(VK_DELETE))
    {
        if (m_iCursorPos >= 0 && m_iCursorPos < bufferLen)
        {
            m_editBuffer.erase(m_iCursorPos, 1);
            changed = true;
        }
    }

    // Escape
    /*if (g_KBMInput.IsKeyPressed(VK_ESCAPE))
    {
        m_editBuffer = m_textBeforeEdit;
        m_bDirectEditing = false;
        m_iDirectEditCooldown = 4;
        setLabel(m_editBuffer.c_str());
        return eDirectEdit_Cancelled;
    }*/
    
    return eDirectEdit_Continue;
}

void UIControl_Label::cancelDirectEdit()
{
	if (m_bDirectEditing)
	{
		m_editBuffer = m_textBeforeEdit;
		m_bDirectEditing = false;
		m_iDirectEditCooldown = 4;
		setLabel(m_editBuffer.c_str(), true);
	}
}

void UIControl_Label::confirmDirectEdit()
{
	if (m_bDirectEditing)
	{
		m_bDirectEditing = false;
		setLabel(m_editBuffer.c_str(), true, true);
	}
}

#endif
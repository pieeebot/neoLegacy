#pragma once

#include "UIControl_Base.h"

class UIControl_Label : public UIControl_Base
{
private:
	bool m_reinitEnabled;

public:
	UIControl_Label();

#ifdef _WINDOWS64
	bool m_bDirectEditing = false;
	wstring m_textBeforeEdit;
	IggyName m_funcSetCaretIndex;
	wstring m_editBuffer;
	int m_iCursorPos;
	int m_iCharLimit;
	int m_iLineLimit = 15;
	int m_iDirectEditCooldown;
	int m_iCaretBlinkTimer;
	bool m_bhasBeenSelected = false;
	bool signing = false;
	int iRealWidth = 0;

	int m_cPosMin = 0;
	int m_cPosMax;
#endif

	virtual bool setupControl(UIScene *scene, IggyValuePath *parent, const string &controlName);

	void init(UIString label, int id = -1);
	virtual void ReInit();

	void disableReinitialisation() { m_reinitEnabled = false; }

	enum EDirectEditResult
	{
		eDirectEdit_Continue,
		eDirectEdit_Confirmed,
		eDirectEdit_Cancelled,
	};

	wstring authorName;

	void beginDirectEdit(int charLimit, bool bSigning, wstring author);
	void UpdateCaretIndex(int index);
	EDirectEditResult tickDirectEdit();
	void cancelDirectEdit();
	void confirmDirectEdit();
};
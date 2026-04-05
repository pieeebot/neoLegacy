#pragma once

#include "UIControl_Base.h"

class UIControl_PageFlip : public UIControl_Base
{
public:
	UIControl_PageFlip();

	virtual bool setupControl(UIScene *scene, IggyValuePath *parent, const string &controlName);

	void init(UIString label, int id);
	//void init(const wstring &label, int id) { init(UIString::CONSTANT(label), id); }

	virtual void ReInit();
};
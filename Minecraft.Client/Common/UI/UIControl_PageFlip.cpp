#include "stdafx.h"
#include "UI.h"
#include "UIControl_PageFlip.h"

UIControl_PageFlip::UIControl_PageFlip()
{
}

bool UIControl_PageFlip::setupControl(UIScene *scene, IggyValuePath *parent, const string &controlName)
{
	UIControl::setControlType(UIControl::ePageFlip);
	bool success = UIControl_Base::setupControl(scene,parent,controlName);

	return success;
}

void UIControl_PageFlip::init(UIString label, int id)
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
	IggyResult out = IggyPlayerCallMethodRS ( m_parentScene->getMovie() , &result, getIggyValuePath() , m_initFunc , 2 , value );

#ifdef __PSVITA__
	// 4J-PB - add this button to the vita touch box list

	switch(m_parentScene->GetParentLayer()->m_iLayer)
	{
	case eUILayer_Error:
	case eUILayer_Fullscreen:
	case eUILayer_Scene:
	case eUILayer_HUD:
		ui.TouchBoxAdd(this,m_parentScene);
		break;
	}
#endif
}

void UIControl_PageFlip::ReInit()
{
	UIControl_Base::ReInit();

	init(m_label, m_id);
}

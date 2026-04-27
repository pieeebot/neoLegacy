#include "UIControl_AchievementsList.h"
#include "stdafx.h"
#include "UI.h"
#include "../../../Minecraft.World/JavaMath.h"

UIControl_AchievementsList::UIControl_AchievementsList()
{
	
}

void UIControl_AchievementsList::init(int id)
{
	m_id = id;

	IggyDataValue result;
	IggyDataValue value[1];
	value[0].type = IGGY_DATATYPE_number;
	value[0].number = id;
	IggyResult out = IggyPlayerCallMethodRS(m_parentScene->getMovie(), &result, getIggyValuePath(), m_initFunc, 1, value);

	//Hardcoded from SWF since it techinically won't ever change, and getting the x and y just doesnt work through the intended way
	m_x = 221;
	m_y = 204;
}

void UIControl_AchievementsList::setCurrentSelection(int iSelection)
{
	IggyDataValue result;
	IggyDataValue value[1];

	value[0].type = IGGY_DATATYPE_number;
	value[0].number = iSelection;
	IggyResult out = IggyPlayerCallMethodRS(m_parentScene->getMovie(), &result, getIggyValuePath(), m_funcHighlightItem, 1, value);
}

bool UIControl_AchievementsList::setupControl(UIScene* scene, IggyValuePath* parent, const string& controlName)
{
	UIControl::setControlType(UIControl::eAchievementList);
	bool success = UIControl_Base::setupControl(scene, parent, controlName);

	//Label specific initialisers
	m_funcAddNewItem = registerFastName(L"addNewItem");
	m_funcHighlightItem = registerFastName(L"HighlightItem");
	m_funcSetTouchFocus = registerFastName(L"SetTouchFocus");
	m_funcEnableButton = registerFastName(L"EnableButton");

	return success;
}

void UIControl_AchievementsList::UpdateControl()
{
	F64 fwidth, fheight;
	IggyValueGetF64RS(getIggyValuePath(), m_nameWidth, nullptr, &fwidth);
	IggyValueGetF64RS(getIggyValuePath(), m_nameHeight, nullptr, &fheight);
	m_width = static_cast<S32>(Math::round(fwidth));
	m_height = static_cast<S32>(Math::round(fheight));
}

void UIControl_AchievementsList::SetTouchFocus(S32 iX, S32 iY, bool bRepeat)
{
	IggyDataValue result;
	IggyDataValue value[3];

	value[0].type = IGGY_DATATYPE_number;
	value[0].number = iX;
	value[1].type = IGGY_DATATYPE_number;
	value[1].number = iY;
	value[2].type = IGGY_DATATYPE_boolean;
	value[2].boolval = bRepeat;

	IggyResult out = IggyPlayerCallMethodRS(m_parentScene->getMovie(), &result, getIggyValuePath(), m_funcSetTouchFocus, 3, value);
}

void UIControl_AchievementsList::addnewItem(int id, wstring textureName)
{
	IggyDataValue result;
	IggyDataValue value[2];
	value[0].type = IGGY_DATATYPE_number;
	value[0].number = id;
	value[1].type = IGGY_DATATYPE_string_UTF16;
	IggyStringUTF16 stringVal;
	stringVal.string = (IggyUTF16*)textureName.c_str();
	stringVal.length = textureName.length();
	value[1].string16 = stringVal;
	IggyResult out = IggyPlayerCallMethodRS(m_parentScene->getMovie(), &result, getIggyValuePath(), m_funcAddNewItem, 2, value);

	++m_itemCount;
}

void UIControl_AchievementsList::EnableButton(int id, bool bEnable) {
	IggyDataValue result;
	IggyDataValue value[2];
	value[0].type = IGGY_DATATYPE_number;
	value[0].number = id;
	value[1].type = IGGY_DATATYPE_boolean;
	value[1].boolval = bEnable;

	IggyResult out = IggyPlayerCallMethodRS(m_parentScene->getMovie(), &result, getIggyValuePath(), m_funcEnableButton, 2, value);
}

void UIControl_AchievementsList::updateChildFocus(int iChild)
{
	m_iCurrentSelection = iChild;
}
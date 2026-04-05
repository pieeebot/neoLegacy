#pragma once

#include "Common/UI/IUIScene_AbstractContainerMenu.h"
#include "..\Minecraft.World\AbstractContainerMenu.h"

class SimpleContainer;

class IUIScene_WritingBookMenu
{
	public:
		void IUIScene_WritingBookMenu::Init(shared_ptr<ItemInstance> itemInstance);

		IUIScene_WritingBookMenu::IUIScene_WritingBookMenu();

		void IUIScene_WritingBookMenu::SaveChanges(bool signBook);

		std::wstring IUIScene_WritingBookMenu::getCurrentPageText();

		shared_ptr<ItemInstance> IUIScene_WritingBookMenu::OnDoneButtonPress();

		void IUIScene_WritingBookMenu::setCurrentPageText(const std::wstring& newText, int indexOverride = -1);

		void IUIScene_WritingBookMenu::ScrollPageRight();
		void IUIScene_WritingBookMenu::ScrollPageLeft();

		

		shared_ptr<ItemInstance> m_itemInstance;
		ItemInstance* m_itemInstanceCopy = new ItemInstance(0, 0, 0);
		ListTag<Tag>* m_currentPage;
		ListTag<Tag> m_currentPageCopy;
		int m_totalPages = 2;
		int m_currentPageIndex = 0;
		int iRealWidth = 0;

		int iPadLocal;

		bool signing = false;
		bool signedBook = false;

		std::wstring m_bookTitle = L"";
		std::wstring m_label = L"";


	protected:
		virtual void updateTooltips();
};
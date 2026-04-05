#include "stdafx.h"
#include <vector>
#include "IUIScene_WritingBookMenu.h"
#include "MultiPlayerLocalPlayer.h"
#include "MultiPlayerLocalPlayer.h"
#include "Common\UI\UI.h"
#include "ClientConnection.h"
#include "../Minecraft.World/CustomPayloadPacket.h"

void IUIScene_WritingBookMenu::Init(shared_ptr<ItemInstance> itemInstance)
{
    //m_itemInstance = make_shared<ItemInstance>(*itemInstance);
    m_itemInstance = itemInstance;

    app.DebugPrintf(("itemInstance hasTag = " + std::to_string(itemInstance->hasTag()) + "\n").c_str());
    app.DebugPrintf(("m_itemInstance hasTag = " + std::to_string(m_itemInstance->hasTag()) + "\n").c_str());

	if (itemInstance->hasTag()) //m_itemInstance->hasTag()
	{
        CompoundTag* itemTag = m_itemInstance->getTag();
        
        if (itemTag->contains(L"pages"))
        {
            ListTag<Tag>* pagesTag = itemTag->getList(L"pages");

            if (pagesTag && pagesTag->size() > 0)
            {
                m_currentPage = pagesTag;
                CompoundTag* firstPage = static_cast<CompoundTag*>(pagesTag->get(0));

                if (firstPage)
                {
                    m_totalPages = pagesTag->size();
                    updateTooltips();
                    return;
                }
            }
        }

	}

    if (!itemInstance) {
        ListTag<Tag>* pagesListTag = new ListTag<Tag>(L"pages");
        m_currentPage = pagesListTag;  // SET IT

		CompoundTag* firstPageTag = new CompoundTag(L"");
		firstPageTag->putString(L"text", L"");

		pagesListTag->add(firstPageTag);

		m_totalPages = 1;
        m_currentPageIndex = 0;

        if (!m_itemInstance->hasTag())
        {
            CompoundTag* rootTag = new CompoundTag(L"");
            rootTag->put(L"pages", pagesListTag);
            itemInstance->setTag(rootTag);
            m_itemInstance = itemInstance;
        }
        else
        {
            itemInstance->getTag()->put(L"pages", pagesListTag);
            m_itemInstance = itemInstance;
        }
        updateTooltips();
        return;
    }

    ListTag<Tag>* pagesListTag = new ListTag<Tag>(L"pages");
    m_currentPage = pagesListTag;

    CompoundTag* firstPageTag = new CompoundTag(L"");
    firstPageTag->putString(L"text", L"");

    pagesListTag->add(firstPageTag);

    m_totalPages = 1;
    updateTooltips();
    return;
}



void IUIScene_WritingBookMenu::SaveChanges(bool signBook)
{
    ListTag<Tag>* pagesTag = static_cast<ListTag<Tag>*>(m_currentPage);
    if (!pagesTag)
        return;

    ByteArrayOutputStream baos;
    DataOutputStream dos(&baos);

    if (!m_itemInstance->hasTag())
    {
        // Create new NBT tag structure
        CompoundTag* rootTag = new CompoundTag(L"");
        rootTag->put(L"pages", pagesTag);
        m_itemInstance->setTag(rootTag);

        
    }
    else
    {
        // Update existing NBT
        CompoundTag* rootTag = m_itemInstance->tag;
        rootTag->put(L"pages", pagesTag);  // Update pages
    }

    if (signBook)
    {
        std::shared_ptr<MultiplayerLocalPlayer> player = Minecraft::GetInstance()->localplayers[iPadLocal];
        std::wstring authorName = player->getName();

        CompoundTag* rootTag = m_itemInstance->getTag();
        rootTag->putString(L"author", authorName);

        // Add title tag
        rootTag->putString(L"title", m_bookTitle);

        Packet::writeItem(m_itemInstance, &dos);
        Minecraft::GetInstance()->localplayers[iPadLocal]->connection->send(std::make_shared<CustomPayloadPacket>(CustomPayloadPacket::CUSTOM_BOOK_SIGN_PACKET, baos.toByteArray()));
        
        return;
    }

    
     Packet::writeItem(m_itemInstance, &dos);
     Minecraft::GetInstance()->localplayers[iPadLocal]->connection->send(std::make_shared<CustomPayloadPacket>(CustomPayloadPacket::CUSTOM_BOOK_PACKET, baos.toByteArray()));
    return;
}

std::wstring IUIScene_WritingBookMenu::getCurrentPageText()
{
    if (!m_currentPage) return L"error";
    int index = m_currentPageIndex;
    if (index < 0) return L"error";

    ListTag<Tag>* pagesList = static_cast<ListTag<Tag>*>(m_currentPage);
    if (index >= pagesList->size()) return L"error";

    CompoundTag* pageTag = static_cast<CompoundTag*>(pagesList->get(index));

    return pageTag->getString(L"text");
}

void IUIScene_WritingBookMenu::setCurrentPageText(const std::wstring& newText, int indexOverride)
{
    if (signing == true) return;
    //if (newText.empty()) return;
    if (!m_currentPage) return;

    int index = m_currentPageIndex;
    if (index < 0) return;

    if (indexOverride >= 0) index = indexOverride;

    ListTag<Tag>* pagesList = static_cast<ListTag<Tag>*>(m_currentPage);
    if (index >= pagesList->size()) return;

    CompoundTag* pageTag = static_cast<CompoundTag*>(pagesList->get(index));

    pageTag->putString(L"text", newText);
}

//Pretty sure im not using this
IUIScene_WritingBookMenu::IUIScene_WritingBookMenu()
{
    m_currentPage = nullptr;

    m_totalPages = 0;
    m_currentPageIndex = 0;
}

void IUIScene_WritingBookMenu::updateTooltips()
{
    ui.SetTooltipText(iPadLocal, eToolTipButtonA, IDS_TOOLTIPS_SELECT);
    ui.SetTooltipText(iPadLocal, eToolTipButtonB, IDS_TOOLTIPS_BACK);
    //ui.SetTooltips(iPadLocal);

    /*enum EToolTipButton
    {
        eToolTipButtonA = 0,
        eToolTipButtonB,
        eToolTipButtonX,
        eToolTipButtonY,
        eToolTipButtonLT,
        eToolTipButtonRT,
        eToolTipButtonLB,
        eToolTipButtonRB,
        eToolTipButtonLS,
        eToolTipButtonRS,
        eToolTipButtonBack,
        eToolTipNumButtons
    };*/

    if ((m_currentPageIndex+1) < m_totalPages) {
        //ui.SetTooltips(iPadLocal, IDS_TOOLTIPS_SELECT, IDS_TOOLTIPS_BACK, -1, -1, -1, -1, -1, IDS_TOOLTIPS_NEXTPAGE);
        ui.SetTooltipText(iPadLocal, eToolTipButtonRB, IDS_TOOLTIPS_NEXTPAGE);
    }
    else if ((m_currentPageIndex + 1) == m_totalPages && !signedBook) {
        //ui.SetTooltips(iPadLocal, IDS_TOOLTIPS_SELECT, IDS_TOOLTIPS_BACK, -1, -1, -1, -1, -1, IDS_TOOLTIPS_ADDPAGE);
		ui.SetTooltipText(iPadLocal, eToolTipButtonRB, IDS_TOOLTIPS_ADDPAGE);
	}
    else if ((m_currentPageIndex + 1) == m_totalPages && signedBook) {
        ui.ShowTooltip(iPadLocal, eToolTipButtonRB, false);
    }
    if (m_currentPageIndex != 0) {
        ui.SetTooltipText(iPadLocal, eToolTipButtonLB, IDS_TOOLTIPS_BACKPAGE);
    }
    else if (m_currentPageIndex == 0) {
		ui.ShowTooltip(iPadLocal, eToolTipButtonLB, false);
	}
}

shared_ptr<ItemInstance> IUIScene_WritingBookMenu::OnDoneButtonPress()
{
    if (0 == 1) //temporary, to check permissions later on
    {
        return nullptr;
    }

    if (!signedBook)
    {
        SaveChanges(false);
        app.DebugPrintf(("m_itemInstance hasTag = " + std::to_string(m_itemInstance->tag->getAllTags()->size()) + "\n").c_str());
    }

    ui.PlayUISFX(eSFX_Back);
}

void IUIScene_WritingBookMenu::ScrollPageRight()
{
    if (m_currentPageIndex >= m_totalPages - 1)
    {
        if (signedBook) return;

        if (m_totalPages < 50)  // Max 50 pages //TODO: Make this a controlled variable if I want to change it in the future
        {
            ListTag<Tag>* pagesTag = static_cast<ListTag<Tag>*>(m_currentPage);
            if (!pagesTag)
                return;

            if (pagesTag->size() >= 50)
                return;

            CompoundTag* newPageTag = new CompoundTag(L"");
            newPageTag->putString(L"text", L"");

            pagesTag->add(newPageTag);

            m_totalPages++;
            
        }
        else {
            return;
        }
    }
    int r = rand() % 3;
    ESoundEffect sfx = static_cast<ESoundEffect>(r + 6);
    ui.PlayUISFX(sfx);
    m_currentPageIndex++;
}

void IUIScene_WritingBookMenu::ScrollPageLeft()
{
    if (!(m_currentPageIndex <= 0))
    {
        m_currentPageIndex--;
        int r = rand() % 3;
        ESoundEffect sfx = static_cast<ESoundEffect>(r + 6);
        ui.PlayUISFX(sfx);
    }
}
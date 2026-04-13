#include "stdafx.h"
#include "ChatScreen.h"
#include "ClientConnection.h"
#include "Font.h"
#include "MultiPlayerLocalPlayer.h"
#include "../Minecraft.World/SharedConstants.h"
#include "../Minecraft.World/StringHelpers.h"
#include "../Minecraft.World/ChatPacket.h"
#include "../Minecraft.World/ArabicShaping.h"

const wstring ChatScreen::allowedChars = SharedConstants::acceptableLetters;
vector<wstring> ChatScreen::s_chatHistory;
int ChatScreen::s_historyIndex = -1;
wstring ChatScreen::s_historyDraft;
int ChatScreen::s_chatIndex = 0;

bool ChatScreen::isAllowedChatChar(wchar_t c)
{
	if (c < 0x20) return false;
	// Block Unicode bidirectional override characters that can be used to
	// spoof chat messages or impersonate players.
	if (c >= 0x202A && c <= 0x202E) return false; // LRE, RLE, PDF, LRO, RLO
	if (c >= 0x2066 && c <= 0x2069) return false; // LRI, RLI, FSI, PDI
	return true;
}

ChatScreen::ChatScreen()
{
	frame = 0;
	cursorIndex = 0;
	s_historyIndex = -1;

    ChatScreen::s_chatIndex = 0;
}

void ChatScreen::init()
{
	Keyboard::enableRepeatEvents(true);
}

void ChatScreen::removed()
{
	Keyboard::enableRepeatEvents(false);
}

void ChatScreen::tick()
{
	frame++;
	if (cursorIndex > static_cast<int>(message.length()))
		cursorIndex = static_cast<int>(message.length());
}

void ChatScreen::handlePasteRequest()
{
	wstring pasted = Screen::getClipboard();
	for (size_t i = 0; i < pasted.length() && static_cast<int>(message.length()) < SharedConstants::maxChatLength; i++)
	{
		if (isAllowedChatChar(pasted[i]))
		{
			message.insert(cursorIndex, 1, pasted[i]);
			cursorIndex++;
		}
	}
}

void ChatScreen::applyHistoryMessage()
{
	message = s_historyIndex >= 0 ? s_chatHistory[s_historyIndex] : s_historyDraft;
	cursorIndex = static_cast<int>(message.length());
}

void ChatScreen::handleHistoryUp()
{
	if (s_chatHistory.empty()) return;
	if (s_historyIndex == -1)
	{
		s_historyDraft = message;
		s_historyIndex = static_cast<int>(s_chatHistory.size()) - 1;
	}
	else if (s_historyIndex > 0)
		s_historyIndex--;
	applyHistoryMessage();
}

void ChatScreen::handleHistoryDown()
{
	if (s_chatHistory.empty()) return;
	if (s_historyIndex < static_cast<int>(s_chatHistory.size()) - 1)
		s_historyIndex++;
	else
		s_historyIndex = -1;
	applyHistoryMessage();
}

int ChatScreen::getChatIndex()
{
    return ChatScreen::s_chatIndex;
}

void ChatScreen::correctChatIndex(int newChatIndex) {
    ChatScreen::s_chatIndex = newChatIndex;
}

void ChatScreen::setWheelValue(int wheel) {
    ChatScreen::s_chatIndex += wheel;
    if (ChatScreen::s_chatIndex < 0) ChatScreen::s_chatIndex = 0;
}

void ChatScreen::keyPressed(wchar_t ch, int eventKey)
{
    if (eventKey == Keyboard::KEY_ESCAPE)
	{
        minecraft->setScreen(nullptr);
        return;
    }
    if (eventKey == Keyboard::KEY_RETURN)
	{
        wstring trim = trimString(message);
        { char buf[64]; sprintf_s(buf, "[CHAT] Sending (%d chars): ", (int)trim.length()); OutputDebugStringA(buf); }
        OutputDebugStringW(trim.c_str());
        OutputDebugStringA("\n");
        if (trim.length() > 0)
		{
            if (!minecraft->handleClientSideCommand(trim))
			{
                MultiplayerLocalPlayer* mplp = dynamic_cast<MultiplayerLocalPlayer*>(minecraft->player.get());
                if (mplp && mplp->connection)
                    mplp->connection->send(shared_ptr<ChatPacket>(new ChatPacket(trim)));
            }
            if (s_chatHistory.empty() || s_chatHistory.back() != trim)
			{
                s_chatHistory.push_back(trim);
                if (s_chatHistory.size() > CHAT_HISTORY_MAX)
                    s_chatHistory.erase(s_chatHistory.begin());
            }
        }
        minecraft->setScreen(nullptr);
        return;
    }
    if (eventKey == Keyboard::KEY_UP)   { handleHistoryUp();   return; }
    if (eventKey == Keyboard::KEY_DOWN) { handleHistoryDown(); return; }
    if (eventKey == Keyboard::KEY_LEFT)
	{
        if (cursorIndex > 0)
            cursorIndex--;
        return;
    }
    if (eventKey == Keyboard::KEY_RIGHT)
	{
        if (cursorIndex < static_cast<int>(message.length()))
            cursorIndex++;
        return;
    }
    if (eventKey == Keyboard::KEY_BACK && cursorIndex > 0)
	{
        message.erase(cursorIndex - 1, 1);
        cursorIndex--;
        return;
    }
    if (isAllowedChatChar(ch) && static_cast<int>(message.length()) < SharedConstants::maxVisibleLength)
	{
        message.insert(cursorIndex, 1, ch);
        cursorIndex++;
    }
}

void ChatScreen::render(int xm, int ym, float a)
{
    fill(2, height - 14, width - 2, height - 2, 0x80000000);
    const wstring prefix = L"> ";
    int x = 4;
    drawString(font, prefix, x, height - 12, 0xe0e0e0);
    x += font->width(prefix);

    // Shape the full message as one unit so letter connections and word order
    // are correct. Track where the logical cursor maps in the visual string.
    int visualCursorPos = 0;
    wstring shaped = shapeArabicText(message, cursorIndex, &visualCursorPos);

    // Render the full shaped message without re-shaping it
    drawStringPreshaped(font, shaped, x, height - 12, 0xe0e0e0);

    // Place the cursor at the correct visual position
    wstring beforeCursorVisual = shaped.substr(0, visualCursorPos);
    int cursorX = x + font->widthPreshaped(beforeCursorVisual);
    if (frame / 6 % 2 == 0)
        drawString(font, L"_", cursorX, height - 12, 0xe0e0e0);

    Screen::render(xm, ym, a);
}

void ChatScreen::mouseClicked(int x, int y, int buttonNum)
{
    if (buttonNum == 0)
	{
        if (minecraft->gui->selectedName != L"")	// 4J - was nullptr comparison
		{
			if (message.length() > 0 && message[message.length()-1]!=L' ')
			{
                message = message.substr(0, cursorIndex) + L" " + message.substr(cursorIndex);
                cursorIndex++;
            }
            size_t nameLen = minecraft->gui->selectedName.length();
            size_t insertLen = (message.length() + nameLen <= SharedConstants::maxChatLength) ? nameLen : (SharedConstants::maxChatLength - message.length());
            if (insertLen > 0)
			{
                message = message.substr(0, cursorIndex) + minecraft->gui->selectedName.substr(0, insertLen) + message.substr(cursorIndex);
                cursorIndex += static_cast<int>(insertLen);
            }
        }
		else
		{
            Screen::mouseClicked(x, y, buttonNum);
        }
    }
}
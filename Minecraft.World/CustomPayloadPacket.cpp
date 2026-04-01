#include "stdafx.h"
#include "InputOutputStream.h"
#include "PacketListener.h"
#include "BasicTypeContainers.h"
#include "CustomPayloadPacket.h"

// Mojang-defined custom packets
const wstring CustomPayloadPacket::CUSTOM_BOOK_PACKET = L"MC|BEdit";
const wstring CustomPayloadPacket::CUSTOM_BOOK_SIGN_PACKET = L"MC|BSign";
const wstring CustomPayloadPacket::TEXTURE_PACK_PACKET = L"MC|TPack";
const wstring CustomPayloadPacket::TRADER_LIST_PACKET = L"MC|TrList";
const wstring CustomPayloadPacket::TRADER_SELECTION_PACKET = L"MC|TrSel";
const wstring CustomPayloadPacket::SET_ADVENTURE_COMMAND_PACKET = L"MC|AdvCdm";
const wstring CustomPayloadPacket::SET_BEACON_PACKET = L"MC|Beacon";
const wstring CustomPayloadPacket::SET_ITEM_NAME_PACKET = L"MC|ItemName";

const wstring CustomPayloadPacket::CIPHER_KEY_CHANNEL = L"MC|CKey";
const wstring CustomPayloadPacket::CIPHER_ACK_CHANNEL = L"MC|CAck";
const wstring CustomPayloadPacket::CIPHER_ON_CHANNEL = L"MC|COn";

const wstring CustomPayloadPacket::IDENTITY_TOKEN_ISSUE = L"MC|CTIssue";
const wstring CustomPayloadPacket::IDENTITY_TOKEN_CHALLENGE = L"MC|CTChallenge";
const wstring CustomPayloadPacket::IDENTITY_TOKEN_RESPONSE = L"MC|CTResponse";

const wstring CustomPayloadPacket::FORK_HELLO_CHANNEL = L"MC|ForkHello";
const wstring CustomPayloadPacket::FORK_PLAYER_LEAVE_CHANNEL = L"MC|ForkPLeave";

CustomPayloadPacket::CustomPayloadPacket()
	: length(0)
{
}

CustomPayloadPacket::CustomPayloadPacket(const wstring &identifier, byteArray data)
{
	this->identifier = identifier;
	this->data = data;
	this->length = 0;

	if (data.data != nullptr)
	{
		length = data.length;

		if (length > Short::MAX_VALUE)
		{
			app.DebugPrintf("Payload may not be larger than 32K\n");
#ifndef _CONTENT_PACKAGE
			__debugbreak();
#endif
			//throw new IllegalArgumentException("Payload may not be larger than 32k");
		}
	}
}

void CustomPayloadPacket::read(DataInputStream *dis)
{
	identifier = readUtf(dis, 20);
	length = dis->readShort();

	if (length > 0 && length <= Short::MAX_VALUE)
	{
		if(data.data != nullptr)
		{
			delete [] data.data;
		}
		data = byteArray(length);
		dis->readFully(data);
	}
}

void CustomPayloadPacket::write(DataOutputStream *dos)
{
	writeUtf(identifier, dos);
	dos->writeShort(static_cast<short>(length));
	if (data.data != nullptr)
	{
		dos->write(data);
	}
}

void CustomPayloadPacket::handle(PacketListener *listener)
{
	listener->handleCustomPayload( shared_from_this() );
}

int CustomPayloadPacket::getEstimatedSize()
{
	return 2 + identifier.length() * 2 + 2 + length;
}
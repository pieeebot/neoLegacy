#pragma once
using namespace std;

#include "Packet.h"

class CustomPayloadPacket : public Packet, public enable_shared_from_this<CustomPayloadPacket>
{
public:

	// Mojang-defined custom packets
	static const wstring CUSTOM_BOOK_PACKET;
	static const wstring CUSTOM_BOOK_SIGN_PACKET;
	static const wstring TEXTURE_PACK_PACKET;
	static const wstring TRADER_LIST_PACKET;
	static const wstring TRADER_SELECTION_PACKET;
	static const wstring SET_ADVENTURE_COMMAND_PACKET;
	static const wstring SET_BEACON_PACKET;
	static const wstring SET_ITEM_NAME_PACKET;

	// Security: stream cipher handshake channels
	static const wstring CIPHER_KEY_CHANNEL;   // server->client: carries 32-byte key (16 AES key + 16 IV)
	static const wstring CIPHER_ACK_CHANNEL;   // client->server: ack (empty payload)
	static const wstring CIPHER_ON_CHANNEL;    // server->client: activation signal (empty payload)

	// Security: identity token channels
	static const wstring IDENTITY_TOKEN_ISSUE;      // server->client: issue new 32-byte token
	static const wstring IDENTITY_TOKEN_CHALLENGE;   // server->client: request stored token
	static const wstring IDENTITY_TOKEN_RESPONSE;    // client->server: present stored token

	// Fork extensions: server capability and player lifecycle
	static const wstring FORK_HELLO_CHANNEL;         // server->client: identifies fork server (empty payload)
	static const wstring FORK_PLAYER_LEAVE_CHANNEL;  // server->client: player disconnected (payload: UTF gamertag)

	wstring identifier;
	int length;
	byteArray data;

	CustomPayloadPacket();
	CustomPayloadPacket(const wstring &identifier, byteArray data);

	virtual void read(DataInputStream *dis);
	virtual void write(DataOutputStream *dos);
	virtual void handle(PacketListener *listener);
	virtual int getEstimatedSize();

public:
	static shared_ptr<Packet> create() { return std::make_shared<CustomPayloadPacket>(); }
	virtual int getId() { return 250; }
};
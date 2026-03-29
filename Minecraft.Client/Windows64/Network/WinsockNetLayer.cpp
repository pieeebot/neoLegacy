// Code implemented by LCEMP, credit if used on other repos
// https://github.com/LCEMP/LCEMP

#include "stdafx.h"

#ifdef _WINDOWS64

#include "WinsockNetLayer.h"
#include "..\..\Common\Network\PlatformNetworkManagerStub.h"
#include "..\..\..\Minecraft.World\Socket.h"
#if defined(MINECRAFT_SERVER_BUILD)
#include "..\..\..\Minecraft.Server\Access\Access.h"
#include "..\..\..\Minecraft.Server\ServerLogManager.h"
#include "..\..\..\Minecraft.Server\ServerLogger.h"
#include "..\..\..\Minecraft.Server\Security\SecurityConfig.h"
#include "..\..\..\Minecraft.Server\Security\RateLimiter.h"
#include "..\..\..\Minecraft.Server\Security\ConnectionCipher.h"
#endif
#include "..\..\..\Minecraft.World\DisconnectPacket.h"
#include "..\..\Minecraft.h"
#include <windns.h>
#pragma comment(lib, "Dnsapi.lib")
#include "..\4JLibs\inc\4J_Profile.h"

#include <string>

static bool RecvExact(SOCKET sock, BYTE* buf, int len);
#if defined(MINECRAFT_SERVER_BUILD)
static bool TryGetNumericRemoteIp(const sockaddr_in &remoteAddress, std::string *outIp);
#endif

// Raw serialized byte patterns for cipher handshake packets (CustomPayloadPacket ID 250).
// Used by recv threads to detect handshake messages at the byte level before packet parsing,
// enabling atomic cipher activation at the exact byte boundary.

// MC|CAck: 7-char channel, empty payload. Client sends this; server recv thread matches it.
static const BYTE kCipherAckPattern[] = {
	0xFA,                                                                   // packet ID 250
	0x00, 0x07,                                                             // channel length = 7
	0x00, 0x4D, 0x00, 0x43, 0x00, 0x7C, 0x00, 0x43, 0x00, 0x41, 0x00, 0x63, 0x00, 0x6B, // "MC|CAck" UTF-16BE
	0x00, 0x00                                                              // data length = 0
};
static const int kCipherAckPatternSize = sizeof(kCipherAckPattern); // 19

// MC|COn: 6-char channel, empty payload. Client recv thread matches this from server.
static const BYTE kCipherOnPattern[] = {
	0xFA,                                                                   // packet ID 250
	0x00, 0x06,                                                             // channel length = 6
	0x00, 0x4D, 0x00, 0x43, 0x00, 0x7C, 0x00, 0x43, 0x00, 0x4F, 0x00, 0x6E, // "MC|COn" UTF-16BE
	0x00, 0x00                                                              // data length = 0
};
static const int kCipherOnPatternSize = sizeof(kCipherOnPattern); // 17

SOCKET WinsockNetLayer::s_listenSocket = INVALID_SOCKET;
SOCKET WinsockNetLayer::s_hostConnectionSocket = INVALID_SOCKET;
HANDLE WinsockNetLayer::s_acceptThread = nullptr;
HANDLE WinsockNetLayer::s_clientRecvThread = nullptr;

bool WinsockNetLayer::s_isHost = false;
bool WinsockNetLayer::s_connected = false;
bool WinsockNetLayer::s_active = false;
bool WinsockNetLayer::s_initialized = false;

BYTE WinsockNetLayer::s_localSmallId = 0;
BYTE WinsockNetLayer::s_hostSmallId = 0;
unsigned int WinsockNetLayer::s_nextSmallId = XUSER_MAX_COUNT;

CRITICAL_SECTION WinsockNetLayer::s_sendLock;
CRITICAL_SECTION WinsockNetLayer::s_connectionsLock;

std::vector<Win64RemoteConnection> WinsockNetLayer::s_connections;

SOCKET WinsockNetLayer::s_advertiseSock = INVALID_SOCKET;
HANDLE WinsockNetLayer::s_advertiseThread = nullptr;
volatile bool WinsockNetLayer::s_advertising = false;
Win64LANBroadcast WinsockNetLayer::s_advertiseData = {};
CRITICAL_SECTION WinsockNetLayer::s_advertiseLock;
int WinsockNetLayer::s_hostGamePort = WIN64_NET_DEFAULT_PORT;

SOCKET WinsockNetLayer::s_discoverySock = INVALID_SOCKET;
HANDLE WinsockNetLayer::s_discoveryThread = nullptr;
volatile bool WinsockNetLayer::s_discovering = false;
CRITICAL_SECTION WinsockNetLayer::s_discoveryLock;
std::vector<Win64LANSession> WinsockNetLayer::s_discoveredSessions;

CRITICAL_SECTION WinsockNetLayer::s_disconnectLock;
std::vector<BYTE> WinsockNetLayer::s_disconnectedSmallIds;

CRITICAL_SECTION WinsockNetLayer::s_freeSmallIdLock;
std::vector<BYTE> WinsockNetLayer::s_freeSmallIds;
SOCKET WinsockNetLayer::s_smallIdToSocket[256];
CRITICAL_SECTION WinsockNetLayer::s_smallIdToSocketLock;

SOCKET WinsockNetLayer::s_splitScreenSocket[XUSER_MAX_COUNT] = { INVALID_SOCKET, INVALID_SOCKET, INVALID_SOCKET, INVALID_SOCKET };
BYTE WinsockNetLayer::s_splitScreenSmallId[XUSER_MAX_COUNT] = { 0xFF, 0xFF, 0xFF, 0xFF };
HANDLE WinsockNetLayer::s_splitScreenRecvThread[XUSER_MAX_COUNT] = {nullptr, nullptr, nullptr, nullptr};

HANDLE WinsockNetLayer::s_joinThread = nullptr;
volatile WinsockNetLayer::eJoinState WinsockNetLayer::s_joinState = WinsockNetLayer::eJoinState_Idle;
volatile int WinsockNetLayer::s_joinAttempt = 0;
volatile bool WinsockNetLayer::s_joinCancel = false;
char WinsockNetLayer::s_joinIP[256] = {};
int WinsockNetLayer::s_joinPort = 0;
BYTE WinsockNetLayer::s_joinAssignedSmallId = 0;
DisconnectPacket::eDisconnectReason WinsockNetLayer::s_joinRejectReason = DisconnectPacket::eDisconnect_Quitting;

ServerRuntime::Security::StreamCipher WinsockNetLayer::s_clientSendCipher;
ServerRuntime::Security::StreamCipher WinsockNetLayer::s_clientRecvCipher;
CRITICAL_SECTION WinsockNetLayer::s_clientCipherLock;
uint8_t WinsockNetLayer::s_clientPendingKey[ServerRuntime::Security::StreamCipher::KEY_SIZE] = {};
bool WinsockNetLayer::s_clientKeyStored = false;

bool g_Win64MultiplayerHost = false;
bool g_Win64MultiplayerJoin = false;
int g_Win64MultiplayerPort = WIN64_NET_DEFAULT_PORT;
char g_Win64MultiplayerIP[256] = "127.0.0.1";
bool g_Win64DedicatedServer = false;
int g_Win64DedicatedServerPort = WIN64_NET_DEFAULT_PORT;
char g_Win64DedicatedServerBindIP[256] = "";
bool g_Win64DedicatedServerLanAdvertise = true;

bool WinsockNetLayer::Initialize()
{
	if (s_initialized) return true;

	WSADATA wsaData;
	int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (result != 0)
	{
		app.DebugPrintf("WSAStartup failed: %d\n", result);
		return false;
	}

	InitializeCriticalSection(&s_sendLock);
	InitializeCriticalSection(&s_connectionsLock);
	InitializeCriticalSection(&s_advertiseLock);
	InitializeCriticalSection(&s_discoveryLock);
	InitializeCriticalSection(&s_disconnectLock);
	InitializeCriticalSection(&s_freeSmallIdLock);
	InitializeCriticalSection(&s_smallIdToSocketLock);
	InitializeCriticalSection(&s_clientCipherLock);
	for (int i = 0; i < 256; i++)
		s_smallIdToSocket[i] = INVALID_SOCKET;

	s_initialized = true;

	// Dedicated Server does not use LAN session discovery and therefore does not initiate discovery.
	if (!g_Win64DedicatedServer)
	{
		StartDiscovery();
	}

	return true;
}

void WinsockNetLayer::Shutdown()
{
	StopAdvertising();
	StopDiscovery();

	s_joinCancel = true;
	if (s_joinThread != nullptr)
	{
		WaitForSingleObject(s_joinThread, 5000);
		CloseHandle(s_joinThread);
		s_joinThread = nullptr;
	}
	s_joinState = eJoinState_Idle;

	s_active = false;
	s_connected = false;

	if (s_listenSocket != INVALID_SOCKET)
	{
		closesocket(s_listenSocket);
		s_listenSocket = INVALID_SOCKET;
	}

	if (s_hostConnectionSocket != INVALID_SOCKET)
	{
		closesocket(s_hostConnectionSocket);
		s_hostConnectionSocket = INVALID_SOCKET;
	}

	// Stop accept loop first so no new RecvThread can be created while shutting down.
	if (s_acceptThread != nullptr)
	{
		WaitForSingleObject(s_acceptThread, 2000);
		CloseHandle(s_acceptThread);
		s_acceptThread = nullptr;
	}

	std::vector<HANDLE> recvThreads;
	EnterCriticalSection(&s_connectionsLock);
	for (size_t i = 0; i < s_connections.size(); i++)
	{
		s_connections[i].active = false;
		if (s_connections[i].tcpSocket != INVALID_SOCKET)
		{
			closesocket(s_connections[i].tcpSocket);
			s_connections[i].tcpSocket = INVALID_SOCKET;
		}
		if (s_connections[i].recvThread != nullptr)
		{
			recvThreads.push_back(s_connections[i].recvThread);
			s_connections[i].recvThread = nullptr;
		}
	}
	LeaveCriticalSection(&s_connectionsLock);

	// Wait for all host-side receive threads to exit before destroying state.
	for (size_t i = 0; i < recvThreads.size(); i++)
	{
		WaitForSingleObject(recvThreads[i], 2000);
		CloseHandle(recvThreads[i]);
	}

	EnterCriticalSection(&s_connectionsLock);
	s_connections.clear();
	LeaveCriticalSection(&s_connectionsLock);

	if (s_clientRecvThread != nullptr)
	{
		WaitForSingleObject(s_clientRecvThread, 2000);
		CloseHandle(s_clientRecvThread);
		s_clientRecvThread = nullptr;
	}

	for (int i = 0; i < XUSER_MAX_COUNT; i++)
	{
		if (s_splitScreenSocket[i] != INVALID_SOCKET)
		{
			closesocket(s_splitScreenSocket[i]);
			s_splitScreenSocket[i] = INVALID_SOCKET;
		}
		if (s_splitScreenRecvThread[i] != nullptr)
		{
			WaitForSingleObject(s_splitScreenRecvThread[i], 2000);
			CloseHandle(s_splitScreenRecvThread[i]);
			s_splitScreenRecvThread[i] = nullptr;
		}
		s_splitScreenSmallId[i] = 0xFF;
	}

	if (s_initialized)
	{
		EnterCriticalSection(&s_disconnectLock);
		s_disconnectedSmallIds.clear();
		LeaveCriticalSection(&s_disconnectLock);

		EnterCriticalSection(&s_freeSmallIdLock);
		s_freeSmallIds.clear();
		LeaveCriticalSection(&s_freeSmallIdLock);

		ResetClientCipher();
		DeleteCriticalSection(&s_clientCipherLock);
		DeleteCriticalSection(&s_sendLock);
		DeleteCriticalSection(&s_connectionsLock);
		DeleteCriticalSection(&s_advertiseLock);
		DeleteCriticalSection(&s_discoveryLock);
		DeleteCriticalSection(&s_disconnectLock);
		DeleteCriticalSection(&s_freeSmallIdLock);
		DeleteCriticalSection(&s_smallIdToSocketLock);
		WSACleanup();
		s_initialized = false;
	}
}

void WinsockNetLayer::StoreClientCipherKey(const uint8_t key[ServerRuntime::Security::StreamCipher::KEY_SIZE])
{
	EnterCriticalSection(&s_clientCipherLock);
	memcpy(s_clientPendingKey, key, ServerRuntime::Security::StreamCipher::KEY_SIZE);
	s_clientKeyStored = true;
	LeaveCriticalSection(&s_clientCipherLock);
}

bool WinsockNetLayer::SendAckAndActivateClientSendCipher()
{
	if (s_hostConnectionSocket == INVALID_SOCKET)
		return false;

	// Atomic: send the MC|CAck plaintext then activate the send cipher, all under s_sendLock.
	// No other send can interleave between the ack and cipher activation.
	EnterCriticalSection(&s_sendLock);

	// Write framed packet: 4-byte length header + ack pattern
	BYTE header[4];
	header[0] = static_cast<BYTE>((kCipherAckPatternSize >> 24) & 0xFF);
	header[1] = static_cast<BYTE>((kCipherAckPatternSize >> 16) & 0xFF);
	header[2] = static_cast<BYTE>((kCipherAckPatternSize >> 8) & 0xFF);
	header[3] = static_cast<BYTE>(kCipherAckPatternSize & 0xFF);

	bool ok = true;
	int totalSent = 0;
	while (ok && totalSent < 4)
	{
		int sent = send(s_hostConnectionSocket, (const char *)header + totalSent, 4 - totalSent, 0);
		if (sent == SOCKET_ERROR || sent == 0) { ok = false; break; }
		totalSent += sent;
	}
	totalSent = 0;
	while (ok && totalSent < kCipherAckPatternSize)
	{
		int sent = send(s_hostConnectionSocket, (const char *)kCipherAckPattern + totalSent, kCipherAckPatternSize - totalSent, 0);
		if (sent == SOCKET_ERROR || sent == 0) { ok = false; break; }
		totalSent += sent;
	}

	if (ok)
	{
		// Activate send cipher immediately after the ack is on the wire
		EnterCriticalSection(&s_clientCipherLock);
		s_clientSendCipher.Initialize(s_clientPendingKey, ServerRuntime::Security::StreamCipher::Client);
		LeaveCriticalSection(&s_clientCipherLock);
		app.DebugPrintf("Client: Send cipher activated (MC|CAck sent)\n");
	}
	else
	{
		// Partial send corrupts the stream - force disconnect to prevent desync
		app.DebugPrintf("Client: MC|CAck send failed, closing connection\n");
		closesocket(s_hostConnectionSocket);
		s_hostConnectionSocket = INVALID_SOCKET;
	}

	LeaveCriticalSection(&s_sendLock);
	return ok;
}

void WinsockNetLayer::ActivateClientRecvCipher()
{
	EnterCriticalSection(&s_clientCipherLock);
	s_clientRecvCipher.Initialize(s_clientPendingKey, ServerRuntime::Security::StreamCipher::Client);
	SecureZeroMemory(s_clientPendingKey, sizeof(s_clientPendingKey));
	s_clientKeyStored = false;
	LeaveCriticalSection(&s_clientCipherLock);
}

void WinsockNetLayer::ResetClientCipher()
{
	EnterCriticalSection(&s_clientCipherLock);
	s_clientSendCipher.Reset();
	s_clientRecvCipher.Reset();
	SecureZeroMemory(s_clientPendingKey, sizeof(s_clientPendingKey));
	s_clientKeyStored = false;
	LeaveCriticalSection(&s_clientCipherLock);
}

bool WinsockNetLayer::TryEncryptClientOutgoing(uint8_t *data, int length)
{
	if (data == nullptr || length <= 0)
		return false;

	EnterCriticalSection(&s_clientCipherLock);
	bool active = s_clientSendCipher.IsActive();
	if (active)
	{
		s_clientSendCipher.Encrypt(data, length);
	}
	LeaveCriticalSection(&s_clientCipherLock);
	return active;
}

#if defined(MINECRAFT_SERVER_BUILD)
bool WinsockNetLayer::SendCOnAndCommitServerCipher(BYTE smallId)
{
	// Verify a pending key exists before sending MC|COn (prevents rogue ack from triggering spurious activation)
	auto &registry = ServerRuntime::Security::GetCipherRegistry();

	SOCKET sock = GetSocketForSmallId(smallId);
	if (sock == INVALID_SOCKET)
		return false;

	// Verify a pending key exists before sending (rejects rogue acks)
	if (!registry.HasPendingKey(smallId))
	{
		app.DebugPrintf("Server: Ignoring MC|CAck for smallId=%d (no pending key)\n", smallId);
		return false;
	}

	// Atomic: send MC|COn plaintext then commit the cipher, all under s_sendLock.
	// No other send to this smallId can happen between MC|COn and CommitCipher.
	EnterCriticalSection(&s_sendLock);

	BYTE header[4];
	header[0] = static_cast<BYTE>((kCipherOnPatternSize >> 24) & 0xFF);
	header[1] = static_cast<BYTE>((kCipherOnPatternSize >> 16) & 0xFF);
	header[2] = static_cast<BYTE>((kCipherOnPatternSize >> 8) & 0xFF);
	header[3] = static_cast<BYTE>(kCipherOnPatternSize & 0xFF);

	bool ok = true;
	int totalSent = 0;
	while (ok && totalSent < 4)
	{
		int sent = send(sock, (const char *)header + totalSent, 4 - totalSent, 0);
		if (sent == SOCKET_ERROR || sent == 0) { ok = false; break; }
		totalSent += sent;
	}
	totalSent = 0;
	while (ok && totalSent < kCipherOnPatternSize)
	{
		int sent = send(sock, (const char *)kCipherOnPattern + totalSent, kCipherOnPatternSize - totalSent, 0);
		if (sent == SOCKET_ERROR || sent == 0) { ok = false; break; }
		totalSent += sent;
	}

	if (ok)
	{
		// Commit AFTER the send - MC|COn is the last plaintext packet
		registry.CommitCipher(smallId);
		app.DebugPrintf("Server: Cipher committed for smallId=%d (MC|COn sent)\n", smallId);
	}
	else
	{
		// Partial send corrupts the stream - force close
		app.DebugPrintf("Server: MC|COn send failed for smallId=%d, closing socket\n", smallId);
		registry.CancelPending(smallId);
		closesocket(sock);
		ClearSocketForSmallId(smallId);
	}

	LeaveCriticalSection(&s_sendLock);
	return ok;
}
#endif

bool WinsockNetLayer::HostGame(int port, const char* bindIp)
{
	if (!s_initialized && !Initialize()) return false;

	s_isHost = true;
	s_localSmallId = 0;
	s_hostSmallId = 0;
	s_nextSmallId = XUSER_MAX_COUNT;
	s_hostGamePort = port;

	EnterCriticalSection(&s_freeSmallIdLock);
	s_freeSmallIds.clear();
	LeaveCriticalSection(&s_freeSmallIdLock);
	EnterCriticalSection(&s_smallIdToSocketLock);
	for (int i = 0; i < 256; i++)
		s_smallIdToSocket[i] = INVALID_SOCKET;
	LeaveCriticalSection(&s_smallIdToSocketLock);

	struct addrinfo hints = {};
	struct addrinfo* result = nullptr;

	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = (bindIp == nullptr || bindIp[0] == 0) ? AI_PASSIVE : 0;

	char portStr[16];
	sprintf_s(portStr, "%d", port);

	const char* resolvedBindIp = (bindIp != nullptr && bindIp[0] != 0) ? bindIp : nullptr;
	int iResult = getaddrinfo(resolvedBindIp, portStr, &hints, &result);
	if (iResult != 0)
	{
		app.DebugPrintf("getaddrinfo failed for %s:%d - %d\n",
			resolvedBindIp != nullptr ? resolvedBindIp : "*",
			port,
			iResult);
		return false;
	}

	s_listenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
	if (s_listenSocket == INVALID_SOCKET)
	{
		app.DebugPrintf("socket() failed: %d\n", WSAGetLastError());
		freeaddrinfo(result);
		return false;
	}

	int opt = 1;
	setsockopt(s_listenSocket, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));

	iResult = ::bind(s_listenSocket, result->ai_addr, static_cast<int>(result->ai_addrlen));
	freeaddrinfo(result);
	if (iResult == SOCKET_ERROR)
	{
		app.DebugPrintf("bind() failed: %d\n", WSAGetLastError());
		closesocket(s_listenSocket);
		s_listenSocket = INVALID_SOCKET;
		return false;
	}

	iResult = listen(s_listenSocket, SOMAXCONN);
	if (iResult == SOCKET_ERROR)
	{
		app.DebugPrintf("listen() failed: %d\n", WSAGetLastError());
		closesocket(s_listenSocket);
		s_listenSocket = INVALID_SOCKET;
		return false;
	}

	s_active = true;
	s_connected = true;

	s_acceptThread = CreateThread(nullptr, 0, AcceptThreadProc, nullptr, 0, nullptr);

	app.DebugPrintf("Win64 LAN: Hosting on %s:%d\n",
		resolvedBindIp != nullptr ? resolvedBindIp : "*",
		port);
	return true;
}

// Resolve a Minecraft SRV record (_minecraft._tcp.<hostname>) to get the actual host and port.
// Returns true if an SRV record was found and outHost/outPort were updated.
// Returns false (no changes) for numeric IPs, missing records, or DNS errors.
static bool ResolveSRV(const char* hostname, char* outHost, size_t outHostSize, int* outPort)
{
	// Skip numeric IPs (starts with digit, contains no letters)
	if (hostname[0] >= '0' && hostname[0] <= '9')
	{
		bool hasLetter = false;
		for (const char* p = hostname; *p; ++p)
			if ((*p >= 'a' && *p <= 'z') || (*p >= 'A' && *p <= 'Z')) { hasLetter = true; break; }
		if (!hasLetter) return false;
	}

	char srvName[300];
	sprintf_s(srvName, "_minecraft._tcp.%s", hostname);

	DNS_RECORD* records = nullptr;
	DNS_STATUS status = DnsQuery_A(srvName, DNS_TYPE_SRV, DNS_QUERY_STANDARD, nullptr, &records, nullptr);
	if (status != 0 || records == nullptr)
		return false;

	// Find the first SRV record
	for (DNS_RECORD* r = records; r != nullptr; r = r->pNext)
	{
		if (r->wType == DNS_TYPE_SRV)
		{
			strncpy_s(outHost, outHostSize, r->Data.SRV.pNameTarget, _TRUNCATE);
			*outPort = r->Data.SRV.wPort;
			DnsRecordListFree(records, DnsFreeRecordList);
			return true;
		}
	}

	DnsRecordListFree(records, DnsFreeRecordList);
	return false;
}

bool WinsockNetLayer::JoinGame(const char* ip, int port)
{
	if (!s_initialized && !Initialize()) return false;

	s_isHost = false;
	s_hostSmallId = 0;
	s_connected = false;
	s_active = false;

	if (s_hostConnectionSocket != INVALID_SOCKET)
	{
		closesocket(s_hostConnectionSocket);
		s_hostConnectionSocket = INVALID_SOCKET;
	}

	// Wait for old client recv thread to fully exit before starting a new connection.
	// Without this, the old thread can read from the new socket (s_hostConnectionSocket
	// is a global) and steal bytes from the new connection's TCP stream, causing
	// packet stream misalignment on reconnect.
	if (s_clientRecvThread != nullptr)
	{
		WaitForSingleObject(s_clientRecvThread, 5000);
		CloseHandle(s_clientRecvThread);
		s_clientRecvThread = nullptr;
	}

	// Try SRV record resolution for hostnames
	char resolvedHost[256];
	int resolvedPort = port;
	strncpy_s(resolvedHost, sizeof(resolvedHost), ip, _TRUNCATE);
	if (ResolveSRV(ip, resolvedHost, sizeof(resolvedHost), &resolvedPort))
		app.DebugPrintf("SRV resolved %s -> %s:%d\n", ip, resolvedHost, resolvedPort);

	struct addrinfo hints = {};
	struct addrinfo* result = nullptr;

	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

	char portStr[16];
	sprintf_s(portStr, "%d", resolvedPort);

	int iResult = getaddrinfo(resolvedHost, portStr, &hints, &result);
	if (iResult != 0)
	{
		app.DebugPrintf("getaddrinfo failed for %s:%d - %d\n", ip, port, iResult);
		return false;
	}

	bool connected = false;
	BYTE assignedSmallId = 0;
	const int maxAttempts = 3;
	const int connectTimeoutSec = 5;

	for (int attempt = 0; attempt < maxAttempts; ++attempt)
	{
		if (s_joinCancel)
		{
			app.DebugPrintf("JoinGame cancelled by user\n");
			break;
		}

		s_hostConnectionSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
		if (s_hostConnectionSocket == INVALID_SOCKET)
		{
			app.DebugPrintf("socket() failed: %d\n", WSAGetLastError());
			break;
		}

		int noDelay = 1;
		setsockopt(s_hostConnectionSocket, IPPROTO_TCP, TCP_NODELAY, (const char*)&noDelay, sizeof(noDelay));

		// Use non-blocking connect with select() timeout so we don't freeze
		// the game for the full OS TCP timeout when the server is unreachable.
		u_long nonBlocking = 1;
		ioctlsocket(s_hostConnectionSocket, FIONBIO, &nonBlocking);

		iResult = connect(s_hostConnectionSocket, result->ai_addr, static_cast<int>(result->ai_addrlen));
		if (iResult == SOCKET_ERROR)
		{
			int err = WSAGetLastError();
			if (err == WSAEWOULDBLOCK)
			{
				fd_set writeSet, errorSet;
				FD_ZERO(&writeSet);
				FD_SET(s_hostConnectionSocket, &writeSet);
				FD_ZERO(&errorSet);
				FD_SET(s_hostConnectionSocket, &errorSet);

				struct timeval tv;
				tv.tv_sec = connectTimeoutSec;
				tv.tv_usec = 0;

				int selectResult = select(0, nullptr, &writeSet, &errorSet, &tv);
				if (selectResult <= 0 || FD_ISSET(s_hostConnectionSocket, &errorSet))
				{
					app.DebugPrintf("connect() to %s:%d timed out or failed (attempt %d/%d)\n", ip, port, attempt + 1, maxAttempts);
					closesocket(s_hostConnectionSocket);
					s_hostConnectionSocket = INVALID_SOCKET;
					continue;
				}
				// Connection succeeded via non-blocking path
			}
			else
			{
				app.DebugPrintf("connect() to %s:%d failed (attempt %d/%d): %d\n", ip, port, attempt + 1, maxAttempts, err);
				closesocket(s_hostConnectionSocket);
				s_hostConnectionSocket = INVALID_SOCKET;
				Sleep(200);
				continue;
			}
		}

		// Restore blocking mode for normal socket I/O
		u_long blocking = 0;
		ioctlsocket(s_hostConnectionSocket, FIONBIO, &blocking);

		// Set a recv timeout so we don't block forever waiting for the small ID
		DWORD recvTimeout = connectTimeoutSec * 1000;
		setsockopt(s_hostConnectionSocket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&recvTimeout, sizeof(recvTimeout));

		BYTE assignBuf[1];
		int bytesRecv = recv(s_hostConnectionSocket, (char*)assignBuf, 1, 0);
		if (bytesRecv != 1)
		{
			app.DebugPrintf("Failed to receive small ID assignment from host (attempt %d/%d)\n", attempt + 1, maxAttempts);
			closesocket(s_hostConnectionSocket);
			s_hostConnectionSocket = INVALID_SOCKET;
			continue;
		}

		if (assignBuf[0] == WIN64_SMALLID_REJECT)
		{
			BYTE rejectBuf[5];
			if (!RecvExact(s_hostConnectionSocket, rejectBuf, 5))
			{
				app.DebugPrintf("Failed to receive reject reason from host\n");
				closesocket(s_hostConnectionSocket);
				s_hostConnectionSocket = INVALID_SOCKET;
				Sleep(200);
				continue;
			}
			// rejectBuf[0] = packet id (255), rejectBuf[1..4] = 4-byte big-endian reason
			int reason = ((rejectBuf[1] & 0xff) << 24) | ((rejectBuf[2] & 0xff) << 16) |
				((rejectBuf[3] & 0xff) << 8) | (rejectBuf[4] & 0xff);
			Minecraft::GetInstance()->connectionDisconnected(ProfileManager.GetPrimaryPad(), (DisconnectPacket::eDisconnectReason)reason);
			closesocket(s_hostConnectionSocket);
			s_hostConnectionSocket = INVALID_SOCKET;
			freeaddrinfo(result);
			return false;
		}

		assignedSmallId = assignBuf[0];
		connected = true;
		break;
	}
	freeaddrinfo(result);

	if (!connected)
	{
		return false;
	}

	// Clear the recv timeout now that the handshake is complete.
	// The recv thread should block indefinitely waiting for data.
	DWORD noTimeout = 0;
	setsockopt(s_hostConnectionSocket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&noTimeout, sizeof(noTimeout));

	s_localSmallId = assignedSmallId;

	// Save the host IP and port so JoinSplitScreen can connect to the same host
	// regardless of how the connection was initiated (UI vs command line).
	strncpy_s(g_Win64MultiplayerIP, sizeof(g_Win64MultiplayerIP), ip, _TRUNCATE);
	g_Win64MultiplayerPort = port;

	app.DebugPrintf("Win64 LAN: Connected to %s:%d, assigned smallId=%d\n", ip, port, s_localSmallId);

	s_active = true;
	s_connected = true;

	s_clientRecvThread = CreateThread(nullptr, 0, ClientRecvThreadProc, nullptr, 0, nullptr);

	return true;
}

bool WinsockNetLayer::BeginJoinGame(const char* ip, int port)
{
	if (!s_initialized && !Initialize()) return false;

	// Clean up any prior join attempt
	CancelJoinGame();
	if (s_joinThread != nullptr)
	{
		WaitForSingleObject(s_joinThread, 5000);
		CloseHandle(s_joinThread);
		s_joinThread = nullptr;
	}

	s_isHost = false;
	s_hostSmallId = 0;
	s_connected = false;
	s_active = false;

	if (s_hostConnectionSocket != INVALID_SOCKET)
	{
		closesocket(s_hostConnectionSocket);
		s_hostConnectionSocket = INVALID_SOCKET;
	}

	if (s_clientRecvThread != nullptr)
	{
		WaitForSingleObject(s_clientRecvThread, 5000);
		CloseHandle(s_clientRecvThread);
		s_clientRecvThread = nullptr;
	}

	strncpy_s(s_joinIP, sizeof(s_joinIP), ip, _TRUNCATE);
	s_joinPort = port;
	s_joinAttempt = 0;
	s_joinCancel = false;
	s_joinAssignedSmallId = 0;
	s_joinRejectReason = DisconnectPacket::eDisconnect_Quitting;
	s_joinState = eJoinState_Connecting;

	s_joinThread = CreateThread(nullptr, 0, JoinThreadProc, nullptr, 0, nullptr);
	if (s_joinThread == nullptr)
	{
		s_joinState = eJoinState_Failed;
		return false;
	}
	return true;
}

DWORD WINAPI WinsockNetLayer::JoinThreadProc(LPVOID param)
{
	// Try SRV record resolution for hostnames
	char resolvedHost[256];
	int resolvedPort = s_joinPort;
	strncpy_s(resolvedHost, sizeof(resolvedHost), s_joinIP, _TRUNCATE);
	if (ResolveSRV(s_joinIP, resolvedHost, sizeof(resolvedHost), &resolvedPort))
		app.DebugPrintf("SRV resolved %s -> %s:%d\n", s_joinIP, resolvedHost, resolvedPort);

	struct addrinfo hints = {}, *result = nullptr;
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

	char portStr[16];
	sprintf_s(portStr, "%d", resolvedPort);

	if (getaddrinfo(resolvedHost, portStr, &hints, &result) != 0)
	{
		app.DebugPrintf("getaddrinfo failed for %s:%d\n", resolvedHost, resolvedPort);
		s_joinState = eJoinState_Failed;
		return 0;
	}

	bool connected = false;
	BYTE assignedSmallId = 0;
	SOCKET sock = INVALID_SOCKET;
	const int connectTimeoutSec = 5;

	for (int attempt = 0; attempt < JOIN_MAX_ATTEMPTS; ++attempt)
	{
		if (s_joinCancel) { freeaddrinfo(result); s_joinState = eJoinState_Cancelled; return 0; }

		s_joinAttempt = attempt + 1;

		sock = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
		if (sock == INVALID_SOCKET) break;

		int noDelay = 1;
		setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, (const char*)&noDelay, sizeof(noDelay));

		// Non-blocking connect with select() timeout
		u_long nonBlocking = 1;
		ioctlsocket(sock, FIONBIO, &nonBlocking);

		int iResult = connect(sock, result->ai_addr, static_cast<int>(result->ai_addrlen));
		if (iResult == SOCKET_ERROR)
		{
			int err = WSAGetLastError();
			if (err == WSAEWOULDBLOCK)
			{
				fd_set writeSet, errorSet;
				FD_ZERO(&writeSet); FD_SET(sock, &writeSet);
				FD_ZERO(&errorSet); FD_SET(sock, &errorSet);
				struct timeval tv = { connectTimeoutSec, 0 };

				int selectResult = select(0, nullptr, &writeSet, &errorSet, &tv);
				if (selectResult <= 0 || FD_ISSET(sock, &errorSet))
				{
					app.DebugPrintf("connect() to %s:%d timed out (attempt %d/%d)\n", s_joinIP, s_joinPort, attempt + 1, JOIN_MAX_ATTEMPTS);
					closesocket(sock); sock = INVALID_SOCKET;
					continue;
				}
			}
			else
			{
				app.DebugPrintf("connect() to %s:%d failed (attempt %d/%d): %d\n", s_joinIP, s_joinPort, attempt + 1, JOIN_MAX_ATTEMPTS, err);
				closesocket(sock); sock = INVALID_SOCKET;
				for (int w = 0; w < 4 && !s_joinCancel; w++) Sleep(50);
				continue;
			}
		}

		// Restore blocking mode
		u_long blocking = 0;
		ioctlsocket(sock, FIONBIO, &blocking);

		// Temporary recv timeout for the handshake only
		DWORD recvTimeout = connectTimeoutSec * 1000;
		setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&recvTimeout, sizeof(recvTimeout));

		BYTE assignBuf[1];
		if (recv(sock, (char*)assignBuf, 1, 0) != 1)
		{
			app.DebugPrintf("Failed to receive small ID assignment from host (attempt %d/%d)\n", attempt + 1, JOIN_MAX_ATTEMPTS);
			closesocket(sock); sock = INVALID_SOCKET;
			continue;
		}

		if (assignBuf[0] == WIN64_SMALLID_REJECT)
		{
			BYTE rejectBuf[5];
			if (!RecvExact(sock, rejectBuf, 5))
			{
				app.DebugPrintf("Failed to receive reject reason from host\n");
				closesocket(sock); sock = INVALID_SOCKET;
				continue;
			}
			int reason = ((rejectBuf[1] & 0xff) << 24) | ((rejectBuf[2] & 0xff) << 16) |
				((rejectBuf[3] & 0xff) << 8) | (rejectBuf[4] & 0xff);
			s_joinRejectReason = (DisconnectPacket::eDisconnectReason)reason;
			closesocket(sock);
			freeaddrinfo(result);
			s_joinState = eJoinState_Rejected;
			return 0;
		}

		assignedSmallId = assignBuf[0];
		connected = true;
		break;
	}
	freeaddrinfo(result);

	if (s_joinCancel)
	{
		if (sock != INVALID_SOCKET) closesocket(sock);
		s_joinState = eJoinState_Cancelled;
		return 0;
	}

	if (!connected)
	{
		s_joinState = eJoinState_Failed;
		return 0;
	}

	// Clear recv timeout before handing socket to recv thread
	DWORD noTimeout = 0;
	setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&noTimeout, sizeof(noTimeout));

	s_hostConnectionSocket = sock;
	s_joinAssignedSmallId = assignedSmallId;
	s_joinState = eJoinState_Success;
	return 0;
}

void WinsockNetLayer::CancelJoinGame()
{
	s_joinCancel = true;

	// Close socket to immediately unblock any in-progress connect/select/recv
	SOCKET sock = s_hostConnectionSocket;
	if (sock != INVALID_SOCKET)
	{
		s_hostConnectionSocket = INVALID_SOCKET;
		closesocket(sock);
	}

	if (s_joinState == eJoinState_Success || s_joinState == eJoinState_Connecting)
	{
		s_joinState = eJoinState_Cancelled;
	}
}

bool WinsockNetLayer::FinalizeJoin()
{
	if (s_joinState != eJoinState_Success)
		return false;

	s_localSmallId = s_joinAssignedSmallId;

	strncpy_s(g_Win64MultiplayerIP, sizeof(g_Win64MultiplayerIP), s_joinIP, _TRUNCATE);
	g_Win64MultiplayerPort = s_joinPort;

	app.DebugPrintf("Win64 LAN: Connected to %s:%d, assigned smallId=%d\n",
		s_joinIP, s_joinPort, s_localSmallId);

	s_active = true;
	s_connected = true;

	s_clientRecvThread = CreateThread(nullptr, 0, ClientRecvThreadProc, nullptr, 0, nullptr);

	if (s_joinThread != nullptr)
	{
		WaitForSingleObject(s_joinThread, 2000);
		CloseHandle(s_joinThread);
		s_joinThread = nullptr;
	}

	s_joinState = eJoinState_Idle;
	return true;
}

WinsockNetLayer::eJoinState WinsockNetLayer::GetJoinState() { return s_joinState; }
int WinsockNetLayer::GetJoinAttempt() { return s_joinAttempt; }
int WinsockNetLayer::GetJoinMaxAttempts() { return JOIN_MAX_ATTEMPTS; }
DisconnectPacket::eDisconnectReason WinsockNetLayer::GetJoinRejectReason() { return s_joinRejectReason; }

bool WinsockNetLayer::SendOnSocket(SOCKET sock, const void* data, int dataSize)
{
	if (sock == INVALID_SOCKET || dataSize <= 0 || dataSize > WIN64_NET_MAX_PACKET_SIZE) return false;

	// TODO: s_sendLock is a single global lock for ALL sockets. If one client's
	// send() blocks (TCP window full, slow WiFi), every other write thread stalls
	// waiting for this lock — no data flows to any player until the slow send
	// completes. This scales badly with player count (8+ players = noticeable).
	// Fix: replace with per-socket locks indexed by smallId (s_perSocketSendLock[256]).
	// The lock only needs to prevent interleaving of header+payload on the SAME socket;
	// sends to different sockets are independent and should never block each other.
	EnterCriticalSection(&s_sendLock);

	BYTE header[4];
	header[0] = static_cast<BYTE>((dataSize >> 24) & 0xFF);
	header[1] = static_cast<BYTE>((dataSize >> 16) & 0xFF);
	header[2] = static_cast<BYTE>((dataSize >> 8) & 0xFF);
	header[3] = static_cast<BYTE>(dataSize & 0xFF);

	int totalSent = 0;
	int toSend = 4;
	while (totalSent < toSend)
	{
		int sent = send(sock, (const char*)header + totalSent, toSend - totalSent, 0);
		if (sent == SOCKET_ERROR || sent == 0)
		{
			LeaveCriticalSection(&s_sendLock);
			return false;
		}
		totalSent += sent;
	}

	totalSent = 0;
	while (totalSent < dataSize)
	{
		int sent = send(sock, static_cast<const char *>(data) + totalSent, dataSize - totalSent, 0);
		if (sent == SOCKET_ERROR || sent == 0)
		{
			LeaveCriticalSection(&s_sendLock);
			return false;
		}
		totalSent += sent;
	}

	LeaveCriticalSection(&s_sendLock);
	return true;
}

bool WinsockNetLayer::SendToSmallId(BYTE targetSmallId, const void* data, int dataSize)
{
	if (!s_active) return false;

	if (s_isHost)
	{
		SOCKET sock = GetSocketForSmallId(targetSmallId);
		if (sock == INVALID_SOCKET) return false;

#if defined(MINECRAFT_SERVER_BUILD)
		// Encrypt outgoing data if a cipher is active for this connection.
		// TryEncryptOutgoing atomically checks and encrypts under a single lock
		// to avoid TOCTOU races with DeactivateCipher on disconnect.
		if (g_Win64DedicatedServer && dataSize > 0)
		{
			std::vector<BYTE> buf(static_cast<const BYTE*>(data),
				static_cast<const BYTE*>(data) + dataSize);
			if (ServerRuntime::Security::GetCipherRegistry().TryEncryptOutgoing(
				targetSmallId, buf.data(), dataSize))
			{
				return SendOnSocket(sock, buf.data(), dataSize);
			}
		}
#endif
		return SendOnSocket(sock, data, dataSize);
	}
	else
	{
		// Client sending to server - encrypt if send cipher is active
		EnterCriticalSection(&s_clientCipherLock);
		if (s_clientSendCipher.IsActive() && dataSize > 0)
		{
			std::vector<BYTE> buf(static_cast<const BYTE*>(data),
				static_cast<const BYTE*>(data) + dataSize);
			s_clientSendCipher.Encrypt(buf.data(), dataSize);
			LeaveCriticalSection(&s_clientCipherLock);
			return SendOnSocket(s_hostConnectionSocket, buf.data(), dataSize);
		}
		LeaveCriticalSection(&s_clientCipherLock);
		return SendOnSocket(s_hostConnectionSocket, data, dataSize);
	}
}

SOCKET WinsockNetLayer::GetSocketForSmallId(BYTE smallId)
{
	EnterCriticalSection(&s_smallIdToSocketLock);
	SOCKET sock = s_smallIdToSocket[smallId];
	LeaveCriticalSection(&s_smallIdToSocketLock);
	return sock;
}

void WinsockNetLayer::ClearSocketForSmallId(BYTE smallId)
{
	EnterCriticalSection(&s_smallIdToSocketLock);
	s_smallIdToSocket[smallId] = INVALID_SOCKET;
	LeaveCriticalSection(&s_smallIdToSocketLock);
}

// Send reject handshake: sentinel 0xFF + DisconnectPacket wire format (1 byte id 255 + 4 byte big-endian reason). Then caller closes socket.
static void SendRejectWithReason(SOCKET clientSocket, DisconnectPacket::eDisconnectReason reason)
{
	BYTE buf[6];
	buf[0] = WIN64_SMALLID_REJECT;
	buf[1] = (BYTE)255; // DisconnectPacket packet id
	int r = (int)reason;
	buf[2] = (BYTE)((r >> 24) & 0xff);
	buf[3] = (BYTE)((r >> 16) & 0xff);
	buf[4] = (BYTE)((r >> 8) & 0xff);
	buf[5] = (BYTE)(r & 0xff);
	send(clientSocket, (const char*)buf, sizeof(buf), 0);
}

static bool RecvExact(SOCKET sock, BYTE* buf, int len)
{
	int totalRecv = 0;
	while (totalRecv < len)
	{
		int r = recv(sock, (char*)buf + totalRecv, len - totalRecv, 0);
		if (r <= 0) return false;
		totalRecv += r;
	}
	return true;
}

#if defined(MINECRAFT_SERVER_BUILD)
static bool TryGetNumericRemoteIp(const sockaddr_in &remoteAddress, std::string *outIp)
{
	if (outIp == nullptr)
	{
		return false;
	}

	outIp->clear();
	char ipBuffer[64] = {};
	const char *ip = inet_ntop(AF_INET, (void *)&remoteAddress.sin_addr, ipBuffer, sizeof(ipBuffer));
	if (ip == nullptr || ip[0] == 0)
	{
		return false;
	}

	*outIp = ip;
	return true;
}

enum EProxyParseResult
{
	eProxyParse_Success,       // Valid PROXY TCP4 header, IP extracted
	eProxyParse_Unknown,       // Valid PROXY UNKNOWN header, no IP available
	eProxyParse_Malformed,     // Invalid header format
	eProxyParse_Timeout,       // Recv timed out
	eProxyParse_SocketError    // Socket error during read
};

/**
 * Parse a PROXY protocol v1 header from the socket.
 * Must be called immediately after accept(), before any game data is read.
 * Sets a 5-second recv timeout, reads the header, restores timeout on all paths.
 */
static EProxyParseResult TryReadProxyProtocolHeader(SOCKET sock, std::string *outSrcIp)
{
	if (outSrcIp != nullptr)
		outSrcIp->clear();

	// Set 5-second recv timeout for the header read
	DWORD timeout = 5000;
	setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char *)&timeout, sizeof(timeout));

	auto restoreTimeout = [sock]() {
		DWORD noTimeout = 0;
		setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char *)&noTimeout, sizeof(noTimeout));
	};

	// Peek at first 6 bytes to check for "PROXY " prefix
	char peekBuf[6];
	int peekResult = recv(sock, peekBuf, 6, MSG_PEEK);
	if (peekResult == 0)
	{
		restoreTimeout();
		return eProxyParse_SocketError;
	}
	if (peekResult == SOCKET_ERROR)
	{
		restoreTimeout();
		int err = WSAGetLastError();
		return (err == WSAETIMEDOUT) ? eProxyParse_Timeout : eProxyParse_SocketError;
	}
	if (peekResult < 6 || memcmp(peekBuf, "PROXY ", 6) != 0)
	{
		restoreTimeout();
		return eProxyParse_Malformed;
	}

	// Consume header byte-by-byte until \r\n (max 107 bytes per PROXY v1 spec)
	char lineBuf[108] = {};
	int lineLen = 0;
	bool foundEnd = false;

	while (lineLen < 107)
	{
		char ch;
		int r = recv(sock, &ch, 1, 0);
		if (r != 1)
		{
			restoreTimeout();
			int err = WSAGetLastError();
			return (r == SOCKET_ERROR && err == WSAETIMEDOUT) ? eProxyParse_Timeout : eProxyParse_SocketError;
		}
		lineBuf[lineLen++] = ch;

		if (lineLen >= 2 && lineBuf[lineLen - 2] == '\r' && lineBuf[lineLen - 1] == '\n')
		{
			foundEnd = true;
			lineBuf[lineLen - 2] = '\0';  // null-terminate, strip \r\n
			break;
		}
	}

	restoreTimeout();

	if (!foundEnd)
	{
		return eProxyParse_Malformed;
	}

	// Parse: "PROXY TCP4 <src_ip> <dst_ip> <src_port> <dst_port>"
	// or:    "PROXY UNKNOWN"
	char *tokens[6] = {};
	int tokenCount = 0;
	char *ctx = nullptr;
	char *tok = strtok_s(lineBuf, " ", &ctx);
	while (tok != nullptr && tokenCount < 6)
	{
		tokens[tokenCount++] = tok;
		tok = strtok_s(nullptr, " ", &ctx);
	}

	if (tokenCount < 2 || strcmp(tokens[0], "PROXY") != 0)
	{
		return eProxyParse_Malformed;
	}

	if (strcmp(tokens[1], "UNKNOWN") == 0)
	{
		return eProxyParse_Unknown;
	}

	if (strcmp(tokens[1], "TCP4") != 0 || tokenCount < 6)
	{
		return eProxyParse_Malformed;
	}

	// Validate src_ip with inet_pton
	struct in_addr addr;
	if (inet_pton(AF_INET, tokens[2], &addr) != 1)
	{
		return eProxyParse_Malformed;
	}

	if (outSrcIp != nullptr)
	{
		*outSrcIp = tokens[2];
	}

	return eProxyParse_Success;
}
#endif

void WinsockNetLayer::HandleDataReceived(BYTE fromSmallId, BYTE toSmallId, unsigned char* data, unsigned int dataSize)
{
	INetworkPlayer* pPlayerFrom = g_NetworkManager.GetPlayerBySmallId(fromSmallId);
	INetworkPlayer* pPlayerTo = g_NetworkManager.GetPlayerBySmallId(toSmallId);

	if (pPlayerFrom == nullptr || pPlayerTo == nullptr)
	{
		app.DebugPrintf("NET RECV: DROPPED %u bytes from=%d to=%d (player NULL: from=%p to=%p)\n",
			dataSize, fromSmallId, toSmallId, pPlayerFrom, pPlayerTo);
		return;
	}

	if (s_isHost)
	{
		::Socket* pSocket = pPlayerFrom->GetSocket();
		if (pSocket != nullptr)
			pSocket->pushDataToQueue(data, dataSize, false);
		else
			app.DebugPrintf("NET RECV: DROPPED %u bytes, host pSocket NULL for from=%d\n", dataSize, fromSmallId);
	}
	else
	{
		::Socket* pSocket = pPlayerTo->GetSocket();
		if (pSocket != nullptr)
			pSocket->pushDataToQueue(data, dataSize, true);
		else
			app.DebugPrintf("NET RECV: DROPPED %u bytes, client pSocket NULL for to=%d\n", dataSize, toSmallId);
	}
}

DWORD WINAPI WinsockNetLayer::AcceptThreadProc(LPVOID param)
{
	while (s_active)
	{
		sockaddr_in remoteAddress;
		ZeroMemory(&remoteAddress, sizeof(remoteAddress));
		int remoteAddressLength = sizeof(remoteAddress);
		SOCKET clientSocket = accept(s_listenSocket, (sockaddr*)&remoteAddress, &remoteAddressLength);
		if (clientSocket == INVALID_SOCKET)
		{
			if (s_active)
				app.DebugPrintf("accept() failed: %d\n", WSAGetLastError());
			break;
		}

		int noDelay = 1;
		setsockopt(clientSocket, IPPROTO_TCP, TCP_NODELAY, (const char*)&noDelay, sizeof(noDelay));

#if defined(MINECRAFT_SERVER_BUILD)
		std::string remoteIp;
		bool hasRemoteIp = TryGetNumericRemoteIp(remoteAddress, &remoteIp);

		// PROXY protocol v1: parse real client IP from tunnel header
		if (g_Win64DedicatedServer && ServerRuntime::Security::GetSettings().proxyProtocol)
		{
			std::string proxiedIp;
			EProxyParseResult proxyResult = TryReadProxyProtocolHeader(clientSocket, &proxiedIp);
			if (proxyResult == eProxyParse_Success)
			{
				ServerRuntime::LogInfof("network", "PROXY: real client IP %s (tunnel: %s)",
					proxiedIp.c_str(), hasRemoteIp ? remoteIp.c_str() : "unknown");
				remoteIp = proxiedIp;
				hasRemoteIp = true;
			}
			else if (proxyResult == eProxyParse_Unknown)
			{
				ServerRuntime::LogInfof("network", "PROXY: UNKNOWN header, keeping tunnel IP");
			}
			else
			{
				ServerRuntime::LogWarnf("network", "PROXY: header parse failed (result=%d) from %s",
					(int)proxyResult, hasRemoteIp ? remoteIp.c_str() : "unknown");
				const char *rejectIp = hasRemoteIp ? remoteIp.c_str() : "unknown";
				ServerRuntime::ServerLogManager::OnRejectedTcpConnection(rejectIp,
					ServerRuntime::ServerLogManager::eTcpRejectReason_InvalidProxyHeader);
				closesocket(clientSocket);
				continue;
			}
		}

		const char *remoteIpForLog = hasRemoteIp ? remoteIp.c_str() : "unknown";
		if (g_Win64DedicatedServer)
		{
			ServerRuntime::ServerLogManager::OnIncomingTcpConnection(remoteIpForLog);
			if (hasRemoteIp && ServerRuntime::Access::IsIpBanned(remoteIp))
			{
				ServerRuntime::ServerLogManager::OnRejectedTcpConnection(remoteIpForLog, ServerRuntime::ServerLogManager::eTcpRejectReason_BannedIp);
				SendRejectWithReason(clientSocket, DisconnectPacket::eDisconnect_Banned);
				closesocket(clientSocket);
				continue;
			}

			// Rate limiting: reject connections that exceed the per-IP sliding window
			if (hasRemoteIp)
			{
				const auto &secSettings = ServerRuntime::Security::GetSettings();
				bool allowed = ServerRuntime::Security::GetGlobalRateLimiter().AllowConnection(
					remoteIp,
					secSettings.rateLimitConnectionsPerWindow,
					secSettings.rateLimitWindowSeconds * 1000);
				if (!allowed)
				{
					ServerRuntime::ServerLogManager::OnRejectedTcpConnection(remoteIpForLog, ServerRuntime::ServerLogManager::eTcpRejectReason_RateLimited);
					closesocket(clientSocket);
					continue;
				}
			}
		}
#endif

		extern QNET_STATE _iQNetStubState;
		if (_iQNetStubState != QNET_STATE_GAME_PLAY)
		{
#if defined(MINECRAFT_SERVER_BUILD)
			if (g_Win64DedicatedServer)
			{
				ServerRuntime::ServerLogManager::OnRejectedTcpConnection(remoteIpForLog, ServerRuntime::ServerLogManager::eTcpRejectReason_GameNotReady);
			}
			else
#endif
			{
				app.DebugPrintf("Win64 LAN: Rejecting connection, game not ready\n");
			}
			closesocket(clientSocket);
			continue;
		}

		extern CPlatformNetworkManagerStub* g_pPlatformNetworkManager;
		if (g_pPlatformNetworkManager != nullptr && !g_pPlatformNetworkManager->CanAcceptMoreConnections())
		{
#if defined(MINECRAFT_SERVER_BUILD)
			if (g_Win64DedicatedServer)
			{
				ServerRuntime::ServerLogManager::OnRejectedTcpConnection(remoteIpForLog, ServerRuntime::ServerLogManager::eTcpRejectReason_ServerFull);
			}
			else
#endif
			{
				app.DebugPrintf("Win64 LAN: Rejecting connection, server at max players\n");
			}
			SendRejectWithReason(clientSocket, DisconnectPacket::eDisconnect_ServerFull);
			closesocket(clientSocket);
			continue;
		}

		BYTE assignedSmallId;
		EnterCriticalSection(&s_freeSmallIdLock);
		if (!s_freeSmallIds.empty())
		{
			assignedSmallId = s_freeSmallIds.back();
			s_freeSmallIds.pop_back();
		}
		else if (s_nextSmallId < (unsigned int)MINECRAFT_NET_MAX_PLAYERS)
		{
			assignedSmallId = (BYTE)s_nextSmallId++;
		}
		else
		{
			LeaveCriticalSection(&s_freeSmallIdLock);
#if defined(MINECRAFT_SERVER_BUILD)
			if (g_Win64DedicatedServer)
			{
				ServerRuntime::ServerLogManager::OnRejectedTcpConnection(remoteIpForLog, ServerRuntime::ServerLogManager::eTcpRejectReason_ServerFull);
			}
			else
#endif
			{
				app.DebugPrintf("Win64 LAN: Server full, rejecting connection\n");
			}
			SendRejectWithReason(clientSocket, DisconnectPacket::eDisconnect_ServerFull);
			closesocket(clientSocket);
			continue;
		}
		LeaveCriticalSection(&s_freeSmallIdLock);

		BYTE assignBuf[1] = { assignedSmallId };
		int sent = send(clientSocket, (const char*)assignBuf, 1, 0);
		if (sent != 1)
		{
			app.DebugPrintf("Failed to send small ID to client\n");
			closesocket(clientSocket);
			PushFreeSmallId(assignedSmallId);
			continue;
		}

		Win64RemoteConnection conn;
		conn.tcpSocket = clientSocket;
		conn.smallId = assignedSmallId;
		conn.active = true;
		conn.recvThread = nullptr;

		EnterCriticalSection(&s_connectionsLock);
		s_connections.push_back(conn);
		int connIdx = static_cast<int>(s_connections.size()) - 1;
		LeaveCriticalSection(&s_connectionsLock);

#if defined(MINECRAFT_SERVER_BUILD)
		if (g_Win64DedicatedServer)
		{
			ServerRuntime::ServerLogManager::OnAcceptedTcpConnection(assignedSmallId, remoteIpForLog);
		}
		else
#endif
		{
			app.DebugPrintf("Win64 LAN: Client connected, assigned smallId=%d\n", assignedSmallId);
		}

		EnterCriticalSection(&s_smallIdToSocketLock);
		s_smallIdToSocket[assignedSmallId] = clientSocket;
		LeaveCriticalSection(&s_smallIdToSocketLock);

		IQNetPlayer* qnetPlayer = &IQNet::m_player[assignedSmallId];

		extern void Win64_SetupRemoteQNetPlayer(IQNetPlayer * player, BYTE smallId, bool isHost, bool isLocal);
		Win64_SetupRemoteQNetPlayer(qnetPlayer, assignedSmallId, false, false);

		extern CPlatformNetworkManagerStub* g_pPlatformNetworkManager;
		g_pPlatformNetworkManager->NotifyPlayerJoined(qnetPlayer);

		DWORD* threadParam = new DWORD;
		*threadParam = connIdx;
		HANDLE hThread = CreateThread(nullptr, 0, RecvThreadProc, threadParam, 0, nullptr);

		EnterCriticalSection(&s_connectionsLock);
		if (connIdx < static_cast<int>(s_connections.size()))
			s_connections[connIdx].recvThread = hThread;
		LeaveCriticalSection(&s_connectionsLock);
	}
	return 0;
}

DWORD WINAPI WinsockNetLayer::RecvThreadProc(LPVOID param)
{
	DWORD connIdx = *static_cast<DWORD *>(param);
	delete static_cast<DWORD *>(param);

	EnterCriticalSection(&s_connectionsLock);
	if (connIdx >= static_cast<DWORD>(s_connections.size()))
	{
		LeaveCriticalSection(&s_connectionsLock);
		return 0;
	}
	SOCKET sock = s_connections[connIdx].tcpSocket;
	BYTE clientSmallId = s_connections[connIdx].smallId;
	LeaveCriticalSection(&s_connectionsLock);

	std::vector<BYTE> recvBuf;
	recvBuf.resize(WIN64_NET_RECV_BUFFER_SIZE);

	while (s_active)
	{
		BYTE header[4];
		if (!RecvExact(sock, header, 4))
		{
			app.DebugPrintf("Win64 LAN: Client smallId=%d disconnected (header)\n", clientSmallId);
			break;
		}

		int packetSize =
			(static_cast<uint32_t>(header[0]) << 24) |
			(static_cast<uint32_t>(header[1]) << 16) |
			(static_cast<uint32_t>(header[2]) << 8) |
			static_cast<uint32_t>(header[3]);

		if (packetSize <= 0 || packetSize > WIN64_NET_MAX_PACKET_SIZE)
		{
			app.DebugPrintf("Win64 LAN: Invalid packet size %d from client smallId=%d (max=%d)\n",
				packetSize,
				clientSmallId,
				(int)WIN64_NET_MAX_PACKET_SIZE);
			break;
		}

		if (static_cast<int>(recvBuf.size()) < packetSize)
		{
			recvBuf.resize(packetSize);
			app.DebugPrintf("Win64 LAN: Resized host recv buffer to %d bytes for client smallId=%d\n", packetSize, clientSmallId);
		}

		if (!RecvExact(sock, &recvBuf[0], packetSize))
		{
			app.DebugPrintf("Win64 LAN: Client smallId=%d disconnected (body)\n", clientSmallId);
			break;
		}

#if defined(MINECRAFT_SERVER_BUILD)
		// Check for MC|CAck cipher handshake (raw byte match, before decryption).
		// The ack is always plaintext - it's the last plaintext packet from the client.
		if (g_Win64DedicatedServer &&
			packetSize == kCipherAckPatternSize &&
			memcmp(&recvBuf[0], kCipherAckPattern, kCipherAckPatternSize) == 0)
		{
			// Atomically send MC|COn plaintext then commit the cipher
			SendCOnAndCommitServerCipher(clientSmallId);
			continue;  // consumed - do not pass to game packet handler
		}

		// Decrypt incoming data if a cipher is active for this connection
		if (g_Win64DedicatedServer)
		{
			ServerRuntime::Security::GetCipherRegistry().DecryptIncoming(clientSmallId, &recvBuf[0], packetSize);
		}
#endif

		HandleDataReceived(clientSmallId, s_hostSmallId, &recvBuf[0], packetSize);
	}

	EnterCriticalSection(&s_connectionsLock);
	for (size_t i = 0; i < s_connections.size(); i++)
	{
		if (s_connections[i].smallId == clientSmallId)
		{
			s_connections[i].active = false;
			if (s_connections[i].tcpSocket != INVALID_SOCKET)
			{
				closesocket(s_connections[i].tcpSocket);
				s_connections[i].tcpSocket = INVALID_SOCKET;
			}
			break;
		}
	}
	LeaveCriticalSection(&s_connectionsLock);

	EnterCriticalSection(&s_disconnectLock);
	s_disconnectedSmallIds.push_back(clientSmallId);
	LeaveCriticalSection(&s_disconnectLock);

	return 0;
}

bool WinsockNetLayer::PopDisconnectedSmallId(BYTE* outSmallId)
{
	bool found = false;
	EnterCriticalSection(&s_disconnectLock);
	if (!s_disconnectedSmallIds.empty())
	{
		*outSmallId = s_disconnectedSmallIds.back();
		s_disconnectedSmallIds.pop_back();
		found = true;
	}
	LeaveCriticalSection(&s_disconnectLock);
	return found;
}

void WinsockNetLayer::PushFreeSmallId(BYTE smallId)
{
#if defined(MINECRAFT_SERVER_BUILD)
	// Clean up any active cipher for this connection
	if (g_Win64DedicatedServer)
	{
		ServerRuntime::Security::GetCipherRegistry().DeactivateCipher(smallId);
	}
#endif

	// SmallIds 0..(XUSER_MAX_COUNT-1) are permanently reserved for the host's
	// local pads and must never be recycled to remote clients.
	if (smallId < (BYTE)XUSER_MAX_COUNT)
		return;

	EnterCriticalSection(&s_freeSmallIdLock);
	// Guard against double-recycle: the reconnect path (queueSmallIdForRecycle) and
	// the DoWork disconnect path can both push the same smallId. If we allow duplicates,
	// AcceptThread will hand out the same smallId to two different connections.
	bool alreadyFree = false;
	for (size_t i = 0; i < s_freeSmallIds.size(); i++)
	{
		if (s_freeSmallIds[i] == smallId) { alreadyFree = true; break; }
	}
	if (!alreadyFree)
		s_freeSmallIds.push_back(smallId);
	LeaveCriticalSection(&s_freeSmallIdLock);
}

void WinsockNetLayer::CloseConnectionBySmallId(BYTE smallId)
{
	EnterCriticalSection(&s_connectionsLock);
	for (size_t i = 0; i < s_connections.size(); i++)
	{
		if (s_connections[i].smallId == smallId && s_connections[i].active && s_connections[i].tcpSocket != INVALID_SOCKET)
		{
			closesocket(s_connections[i].tcpSocket);
			s_connections[i].tcpSocket = INVALID_SOCKET;
			app.DebugPrintf("Win64 LAN: Force-closed TCP connection for smallId=%d\n", smallId);
			break;
		}
	}
	LeaveCriticalSection(&s_connectionsLock);
}

BYTE WinsockNetLayer::GetSplitScreenSmallId(int padIndex)
{
	if (padIndex <= 0 || padIndex >= XUSER_MAX_COUNT) return 0xFF;
	return s_splitScreenSmallId[padIndex];
}

SOCKET WinsockNetLayer::GetLocalSocket(BYTE senderSmallId)
{
	if (senderSmallId == s_localSmallId)
		return s_hostConnectionSocket;
	for (int i = 1; i < XUSER_MAX_COUNT; i++)
	{
		if (s_splitScreenSmallId[i] == senderSmallId && s_splitScreenSocket[i] != INVALID_SOCKET)
			return s_splitScreenSocket[i];
	}
	return INVALID_SOCKET;
}

bool WinsockNetLayer::JoinSplitScreen(int padIndex, BYTE* outSmallId)
{
	if (!s_active || s_isHost || padIndex <= 0 || padIndex >= XUSER_MAX_COUNT)
		return false;

	if (s_splitScreenSocket[padIndex] != INVALID_SOCKET)
	{
		return false;
	}

	struct addrinfo hints = {};
	struct addrinfo* result = nullptr;
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

	char portStr[16];
	sprintf_s(portStr, "%d", g_Win64MultiplayerPort);
	if (getaddrinfo(g_Win64MultiplayerIP, portStr, &hints, &result) != 0 || result == nullptr)
	{
		app.DebugPrintf("Win64 LAN: Split-screen getaddrinfo failed for %s:%d\n", g_Win64MultiplayerIP, g_Win64MultiplayerPort);
		return false;
	}

	SOCKET sock = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
	if (sock == INVALID_SOCKET)
	{
		freeaddrinfo(result);
		return false;
	}

	int noDelay = 1;
	setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, (const char*)&noDelay, sizeof(noDelay));

	if (connect(sock, result->ai_addr, (int)result->ai_addrlen) == SOCKET_ERROR)
	{
		app.DebugPrintf("Win64 LAN: Split-screen connect() failed: %d\n", WSAGetLastError());
		closesocket(sock);
		freeaddrinfo(result);
		return false;
	}
	freeaddrinfo(result);

	BYTE assignBuf[1];
	if (!RecvExact(sock, assignBuf, 1))
	{
		app.DebugPrintf("Win64 LAN: Split-screen failed to receive smallId\n");
		closesocket(sock);
		return false;
	}

	if (assignBuf[0] == WIN64_SMALLID_REJECT)
	{
		BYTE rejectBuf[5];
		RecvExact(sock, rejectBuf, 5);
		app.DebugPrintf("Win64 LAN: Split-screen connection rejected\n");
		closesocket(sock);
		return false;
	}

	BYTE assignedSmallId = assignBuf[0];
	s_splitScreenSocket[padIndex] = sock;
	s_splitScreenSmallId[padIndex] = assignedSmallId;
	*outSmallId = assignedSmallId;

	app.DebugPrintf("Win64 LAN: Split-screen pad %d connected, assigned smallId=%d\n", padIndex, assignedSmallId);

	int* threadParam = new int;
	*threadParam = padIndex;
	s_splitScreenRecvThread[padIndex] = CreateThread(nullptr, 0, SplitScreenRecvThreadProc, threadParam, 0, nullptr);
	if (s_splitScreenRecvThread[padIndex] == nullptr)
	{
		delete threadParam;
		closesocket(sock);
		s_splitScreenSocket[padIndex] = INVALID_SOCKET;
		s_splitScreenSmallId[padIndex] = 0xFF;
		app.DebugPrintf("Win64 LAN: CreateThread failed for split-screen pad %d\n", padIndex);
		return false;
	}

	return true;
}

void WinsockNetLayer::CloseSplitScreenConnection(int padIndex)
{
	if (padIndex <= 0 || padIndex >= XUSER_MAX_COUNT) return;

	if (s_splitScreenSocket[padIndex] != INVALID_SOCKET)
	{
		closesocket(s_splitScreenSocket[padIndex]);
		s_splitScreenSocket[padIndex] = INVALID_SOCKET;
	}
	s_splitScreenSmallId[padIndex] = 0xFF;
	if (s_splitScreenRecvThread[padIndex] != nullptr)
	{
		WaitForSingleObject(s_splitScreenRecvThread[padIndex], 2000);
		CloseHandle(s_splitScreenRecvThread[padIndex]);
		s_splitScreenRecvThread[padIndex] = nullptr;
	}
}

DWORD WINAPI WinsockNetLayer::SplitScreenRecvThreadProc(LPVOID param)
{
	int padIndex = *(int*)param;
	delete (int*)param;

	SOCKET sock = s_splitScreenSocket[padIndex];
	BYTE localSmallId = s_splitScreenSmallId[padIndex];
	std::vector<BYTE> recvBuf;
	recvBuf.resize(WIN64_NET_RECV_BUFFER_SIZE);

	while (s_active && s_splitScreenSocket[padIndex] != INVALID_SOCKET)
	{
		BYTE header[4];
		if (!RecvExact(sock, header, 4))
		{
			app.DebugPrintf("Win64 LAN: Split-screen pad %d disconnected from host\n", padIndex);
			break;
		}

		int packetSize = ((uint32_t)header[0] << 24) | ((uint32_t)header[1] << 16) |
			((uint32_t)header[2] << 8) | ((uint32_t)header[3]);
		if (packetSize <= 0 || packetSize > WIN64_NET_MAX_PACKET_SIZE)
		{
			app.DebugPrintf("Win64 LAN: Split-screen pad %d invalid packet size %d\n", padIndex, packetSize);
			break;
		}

		if ((int)recvBuf.size() < packetSize)
			recvBuf.resize(packetSize);

		if (!RecvExact(sock, &recvBuf[0], packetSize))
		{
			app.DebugPrintf("Win64 LAN: Split-screen pad %d disconnected from host (body)\n", padIndex);
			break;
		}

		HandleDataReceived(s_hostSmallId, localSmallId, &recvBuf[0], packetSize);
	}

	EnterCriticalSection(&s_disconnectLock);
	s_disconnectedSmallIds.push_back(localSmallId);
	LeaveCriticalSection(&s_disconnectLock);

	return 0;
}

DWORD WINAPI WinsockNetLayer::ClientRecvThreadProc(LPVOID param)
{
	std::vector<BYTE> recvBuf;
	recvBuf.resize(WIN64_NET_RECV_BUFFER_SIZE);

	while (s_active && s_hostConnectionSocket != INVALID_SOCKET)
	{
		BYTE header[4];
		if (!RecvExact(s_hostConnectionSocket, header, 4))
		{
			app.DebugPrintf("Win64 LAN: Disconnected from host (header)\n");
			break;
		}

		int packetSize = (header[0] << 24) | (header[1] << 16) | (header[2] << 8) | header[3];

		if (packetSize <= 0 || packetSize > WIN64_NET_MAX_PACKET_SIZE)
		{
			app.DebugPrintf("Win64 LAN: Invalid packet size %d from host (max=%d)\n",
				packetSize,
				(int)WIN64_NET_MAX_PACKET_SIZE);
			break;
		}

		if (static_cast<int>(recvBuf.size()) < packetSize)
		{
			recvBuf.resize(packetSize);
			app.DebugPrintf("Win64 LAN: Resized client recv buffer to %d bytes\n", packetSize);
		}

		if (!RecvExact(s_hostConnectionSocket, &recvBuf[0], packetSize))
		{
			app.DebugPrintf("Win64 LAN: Disconnected from host (body)\n");
			break;
		}

		// Check for MC|COn cipher activation signal (raw byte match, before decryption).
		// This is always sent in plaintext as the last plaintext packet from the server.
		if (packetSize == kCipherOnPatternSize &&
			memcmp(&recvBuf[0], kCipherOnPattern, kCipherOnPatternSize) == 0)
		{
			ActivateClientRecvCipher();
			app.DebugPrintf("Client: Recv cipher activated (MC|COn received)\n");
			continue;  // consumed - do not pass to game packet handler
		}

		// Decrypt incoming data if recv cipher is active
		EnterCriticalSection(&s_clientCipherLock);
		if (s_clientRecvCipher.IsActive())
		{
			s_clientRecvCipher.Decrypt(&recvBuf[0], packetSize);
		}
		LeaveCriticalSection(&s_clientCipherLock);

		HandleDataReceived(s_hostSmallId, s_localSmallId, &recvBuf[0], packetSize);
	}

	s_connected = false;
	ResetClientCipher();
	return 0;
}

bool WinsockNetLayer::StartAdvertising(int gamePort, const wchar_t* hostName, unsigned int gameSettings, unsigned int texPackId, unsigned char subTexId, unsigned short netVer)
{
	if (s_advertising) return true;
	if (!s_initialized) return false;

	EnterCriticalSection(&s_advertiseLock);
	memset(&s_advertiseData, 0, sizeof(s_advertiseData));
	s_advertiseData.magic = WIN64_LAN_BROADCAST_MAGIC;
	s_advertiseData.netVersion = netVer;
	s_advertiseData.gamePort = static_cast<WORD>(gamePort);
	wcsncpy_s(s_advertiseData.hostName, 32, hostName, _TRUNCATE);
	s_advertiseData.playerCount = 1;
	s_advertiseData.maxPlayers = MINECRAFT_NET_MAX_PLAYERS;
	s_advertiseData.gameHostSettings = gameSettings;
	s_advertiseData.texturePackParentId = texPackId;
	s_advertiseData.subTexturePackId = subTexId;
	s_advertiseData.isJoinable = 0;
	s_hostGamePort = gamePort;
	LeaveCriticalSection(&s_advertiseLock);

	s_advertiseSock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (s_advertiseSock == INVALID_SOCKET)
	{
		app.DebugPrintf("Win64 LAN: Failed to create advertise socket: %d\n", WSAGetLastError());
		return false;
	}

	BOOL broadcast = TRUE;
	setsockopt(s_advertiseSock, SOL_SOCKET, SO_BROADCAST, (const char*)&broadcast, sizeof(broadcast));

	s_advertising = true;
	s_advertiseThread = CreateThread(nullptr, 0, AdvertiseThreadProc, nullptr, 0, nullptr);

	app.DebugPrintf("Win64 LAN: Started advertising on UDP port %d\n", WIN64_LAN_DISCOVERY_PORT);
	return true;
}

void WinsockNetLayer::StopAdvertising()
{
	s_advertising = false;

	if (s_advertiseSock != INVALID_SOCKET)
	{
		closesocket(s_advertiseSock);
		s_advertiseSock = INVALID_SOCKET;
	}

	if (s_advertiseThread != nullptr)
	{
		WaitForSingleObject(s_advertiseThread, 2000);
		CloseHandle(s_advertiseThread);
		s_advertiseThread = nullptr;
	}
}

void WinsockNetLayer::UpdateAdvertisePlayerCount(BYTE count)
{
	EnterCriticalSection(&s_advertiseLock);
	s_advertiseData.playerCount = count;
	LeaveCriticalSection(&s_advertiseLock);
}

void WinsockNetLayer::UpdateAdvertiseMaxPlayers(BYTE maxPlayers)
{
	EnterCriticalSection(&s_advertiseLock);
	s_advertiseData.maxPlayers = maxPlayers;
	LeaveCriticalSection(&s_advertiseLock);
}

void WinsockNetLayer::UpdateAdvertiseJoinable(bool joinable)
{
	EnterCriticalSection(&s_advertiseLock);
	s_advertiseData.isJoinable = joinable ? 1 : 0;
	LeaveCriticalSection(&s_advertiseLock);
}

DWORD WINAPI WinsockNetLayer::AdvertiseThreadProc(LPVOID param)
{
	struct sockaddr_in broadcastAddr;
	memset(&broadcastAddr, 0, sizeof(broadcastAddr));
	broadcastAddr.sin_family = AF_INET;
	broadcastAddr.sin_port = htons(WIN64_LAN_DISCOVERY_PORT);
	broadcastAddr.sin_addr.s_addr = INADDR_BROADCAST;

	while (s_advertising)
	{
		EnterCriticalSection(&s_advertiseLock);
		Win64LANBroadcast data = s_advertiseData;
		LeaveCriticalSection(&s_advertiseLock);

		int sent = sendto(s_advertiseSock, (const char*)&data, sizeof(data), 0,
			(struct sockaddr*)&broadcastAddr, sizeof(broadcastAddr));

		if (sent == SOCKET_ERROR && s_advertising)
		{
			app.DebugPrintf("Win64 LAN: Broadcast sendto failed: %d\n", WSAGetLastError());
		}

		Sleep(1000);
	}

	return 0;
}

bool WinsockNetLayer::StartDiscovery()
{
	if (s_discovering) return true;
	if (!s_initialized) return false;

	s_discoverySock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (s_discoverySock == INVALID_SOCKET)
	{
		app.DebugPrintf("Win64 LAN: Failed to create discovery socket: %d\n", WSAGetLastError());
		return false;
	}

	BOOL reuseAddr = TRUE;
	setsockopt(s_discoverySock, SOL_SOCKET, SO_REUSEADDR, (const char*)&reuseAddr, sizeof(reuseAddr));

	struct sockaddr_in bindAddr;
	memset(&bindAddr, 0, sizeof(bindAddr));
	bindAddr.sin_family = AF_INET;
	bindAddr.sin_port = htons(WIN64_LAN_DISCOVERY_PORT);
	bindAddr.sin_addr.s_addr = INADDR_ANY;

	if (::bind(s_discoverySock, (struct sockaddr*)&bindAddr, sizeof(bindAddr)) == SOCKET_ERROR)
	{
		app.DebugPrintf("Win64 LAN: Discovery bind failed: %d\n", WSAGetLastError());
		closesocket(s_discoverySock);
		s_discoverySock = INVALID_SOCKET;
		return false;
	}

	DWORD timeout = 500;
	setsockopt(s_discoverySock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout));

	s_discovering = true;
	s_discoveryThread = CreateThread(nullptr, 0, DiscoveryThreadProc, nullptr, 0, nullptr);

	app.DebugPrintf("Win64 LAN: Listening for LAN games on UDP port %d\n", WIN64_LAN_DISCOVERY_PORT);
	return true;
}

void WinsockNetLayer::StopDiscovery()
{
	s_discovering = false;

	if (s_discoverySock != INVALID_SOCKET)
	{
		closesocket(s_discoverySock);
		s_discoverySock = INVALID_SOCKET;
	}

	if (s_discoveryThread != nullptr)
	{
		WaitForSingleObject(s_discoveryThread, 2000);
		CloseHandle(s_discoveryThread);
		s_discoveryThread = nullptr;
	}

	EnterCriticalSection(&s_discoveryLock);
	s_discoveredSessions.clear();
	LeaveCriticalSection(&s_discoveryLock);
}

std::vector<Win64LANSession> WinsockNetLayer::GetDiscoveredSessions()
{
	std::vector<Win64LANSession> result;
	EnterCriticalSection(&s_discoveryLock);
	result = s_discoveredSessions;
	LeaveCriticalSection(&s_discoveryLock);
	return result;
}

DWORD WINAPI WinsockNetLayer::DiscoveryThreadProc(LPVOID param)
{
	char recvBuf[512];

	while (s_discovering)
	{
		struct sockaddr_in senderAddr;
		int senderLen = sizeof(senderAddr);

		int recvLen = recvfrom(s_discoverySock, recvBuf, sizeof(recvBuf), 0,
			(struct sockaddr*)&senderAddr, &senderLen);

		if (recvLen == SOCKET_ERROR)
		{
			continue;
		}

		if (recvLen < static_cast<int>(sizeof(Win64LANBroadcast)))
			continue;

		Win64LANBroadcast* broadcast = (Win64LANBroadcast*)recvBuf;
		if (broadcast->magic != WIN64_LAN_BROADCAST_MAGIC)
			continue;

		char senderIP[64];
		inet_ntop(AF_INET, &senderAddr.sin_addr, senderIP, sizeof(senderIP));

		DWORD now = GetTickCount();

		EnterCriticalSection(&s_discoveryLock);

		bool found = false;
		for (size_t i = 0; i < s_discoveredSessions.size(); i++)
		{
			if (strcmp(s_discoveredSessions[i].hostIP, senderIP) == 0 &&
				s_discoveredSessions[i].hostPort == static_cast<int>(broadcast->gamePort))
			{
				s_discoveredSessions[i].netVersion = broadcast->netVersion;
				wcsncpy_s(s_discoveredSessions[i].hostName, 32, broadcast->hostName, _TRUNCATE);
				s_discoveredSessions[i].playerCount = broadcast->playerCount;
				s_discoveredSessions[i].maxPlayers = broadcast->maxPlayers;
				s_discoveredSessions[i].gameHostSettings = broadcast->gameHostSettings;
				s_discoveredSessions[i].texturePackParentId = broadcast->texturePackParentId;
				s_discoveredSessions[i].subTexturePackId = broadcast->subTexturePackId;
				s_discoveredSessions[i].isJoinable = (broadcast->isJoinable != 0);
				s_discoveredSessions[i].lastSeenTick = now;
				found = true;
				break;
			}
		}

		if (!found)
		{
			Win64LANSession session;
			memset(&session, 0, sizeof(session));
			strncpy_s(session.hostIP, sizeof(session.hostIP), senderIP, _TRUNCATE);
			session.hostPort = static_cast<int>(broadcast->gamePort);
			session.netVersion = broadcast->netVersion;
			wcsncpy_s(session.hostName, 32, broadcast->hostName, _TRUNCATE);
			session.playerCount = broadcast->playerCount;
			session.maxPlayers = broadcast->maxPlayers;
			session.gameHostSettings = broadcast->gameHostSettings;
			session.texturePackParentId = broadcast->texturePackParentId;
			session.subTexturePackId = broadcast->subTexturePackId;
			session.isJoinable = (broadcast->isJoinable != 0);
			session.lastSeenTick = now;
			s_discoveredSessions.push_back(session);

			app.DebugPrintf("Win64 LAN: Discovered game \"%ls\" at %s:%d\n",
				session.hostName, session.hostIP, session.hostPort);
		}

		for (size_t i = s_discoveredSessions.size(); i > 0; i--)
		{
			if (now - s_discoveredSessions[i - 1].lastSeenTick > 5000)
			{
				app.DebugPrintf("Win64 LAN: Session \"%ls\" at %s timed out\n",
					s_discoveredSessions[i - 1].hostName, s_discoveredSessions[i - 1].hostIP);
				s_discoveredSessions.erase(s_discoveredSessions.begin() + (i - 1));
			}
		}

		LeaveCriticalSection(&s_discoveryLock);
	}

	return 0;
}

#endif

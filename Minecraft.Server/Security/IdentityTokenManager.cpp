#include "stdafx.h"
#include "IdentityTokenManager.h"

#ifdef _WINDOWS64
#include <bcrypt.h>
#endif

#include "..\Common\FileUtils.h"
#include "..\Common\StringUtils.h"
#include "..\ServerLogger.h"
#include "..\vendor\nlohmann\json.hpp"

#include <algorithm>

namespace ServerRuntime
{
	namespace Security
	{
		using OrderedJson = nlohmann::ordered_json;

		IdentityTokenManager::IdentityTokenManager()
			: m_initialized(false)
		{
			InitializeCriticalSection(&m_lock);
		}

		IdentityTokenManager::~IdentityTokenManager()
		{
			DeleteCriticalSection(&m_lock);
		}

		static std::string BytesToBase64(const uint8_t *data, int length)
		{
			static const char kTable[] =
				"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
			std::string out;
			out.reserve(((length + 2) / 3) * 4);
			for (int i = 0; i < length; i += 3)
			{
				uint32_t n = static_cast<uint32_t>(data[i]) << 16;
				if (i + 1 < length) n |= static_cast<uint32_t>(data[i + 1]) << 8;
				if (i + 2 < length) n |= static_cast<uint32_t>(data[i + 2]);
				out.push_back(kTable[(n >> 18) & 0x3F]);
				out.push_back(kTable[(n >> 12) & 0x3F]);
				out.push_back((i + 1 < length) ? kTable[(n >> 6) & 0x3F] : '=');
				out.push_back((i + 2 < length) ? kTable[n & 0x3F] : '=');
			}
			return out;
		}

		static bool Base64ToBytes(const std::string &encoded, std::vector<uint8_t> &out)
		{
			static const int kDecodeTable[128] = {
				-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
				-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
				-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,62,-1,-1,-1,63,
				52,53,54,55,56,57,58,59,60,61,-1,-1,-1,-1,-1,-1,
				-1, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,
				15,16,17,18,19,20,21,22,23,24,25,-1,-1,-1,-1,-1,
				-1,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,
				41,42,43,44,45,46,47,48,49,50,51,-1,-1,-1,-1,-1
			};
			out.clear();
			out.reserve(encoded.size() * 3 / 4);
			uint32_t buf = 0;
			int bits = 0;
			for (char c : encoded)
			{
				if (c == '=') break;
				if (c < 0 || c >= 128 || kDecodeTable[(int)c] < 0) return false;
				buf = (buf << 6) | kDecodeTable[(int)c];
				bits += 6;
				if (bits >= 8)
				{
					bits -= 8;
					out.push_back(static_cast<uint8_t>((buf >> bits) & 0xFF));
				}
			}
			return true;
		}

		static std::string FormatXuid(PlayerUID xuid)
		{
			char buffer[32] = {};
			sprintf_s(buffer, sizeof(buffer), "0x%016llx", (unsigned long long)xuid);
			return buffer;
		}

		bool IdentityTokenManager::Initialize(const std::string &filePath)
		{
			EnterCriticalSection(&m_lock);
			m_filePath = filePath;
			m_tokens.clear();
			bool ok = Load();
			m_initialized = true;
			LeaveCriticalSection(&m_lock);

			if (ok)
			{
				LogInfof("security", "loaded %u identity tokens from %s",
					(unsigned)m_tokens.size(), filePath.c_str());
			}
			else
			{
				LogInfof("security", "no existing identity tokens found, starting fresh");
			}
			return true;
		}

		void IdentityTokenManager::Shutdown()
		{
			EnterCriticalSection(&m_lock);
			m_tokens.clear();
			m_initialized = false;
			LeaveCriticalSection(&m_lock);
		}

		bool IdentityTokenManager::HasToken(PlayerUID xuid) const
		{
			EnterCriticalSection(&m_lock);
			bool found = m_tokens.find(xuid) != m_tokens.end();
			LeaveCriticalSection(&m_lock);
			return found;
		}

		bool IdentityTokenManager::GetToken(PlayerUID xuid, uint8_t outToken[TOKEN_SIZE]) const
		{
			EnterCriticalSection(&m_lock);
			auto it = m_tokens.find(xuid);
			if (it == m_tokens.end() || it->second.size() != TOKEN_SIZE)
			{
				LeaveCriticalSection(&m_lock);
				return false;
			}
			memcpy(outToken, it->second.data(), TOKEN_SIZE);
			LeaveCriticalSection(&m_lock);
			return true;
		}

		bool IdentityTokenManager::IssueToken(PlayerUID xuid, uint8_t outToken[TOKEN_SIZE])
		{
			// Generate a random 32-byte identity token
			uint8_t token[TOKEN_SIZE];
#ifdef _WINDOWS64
			NTSTATUS status = BCryptGenRandom(nullptr, token, TOKEN_SIZE,
				BCRYPT_USE_SYSTEM_PREFERRED_RNG);
			if (!BCRYPT_SUCCESS(status))
			{
				SecureZeroMemory(token, sizeof(token));
				return false;
			}
#else
			for (int i = 0; i < TOKEN_SIZE; ++i)
				token[i] = static_cast<uint8_t>(rand() & 0xFF);
#endif

			EnterCriticalSection(&m_lock);
			m_tokens[xuid] = std::vector<uint8_t>(token, token + TOKEN_SIZE);
			bool saved = Save();
			LeaveCriticalSection(&m_lock);

			if (saved)
			{
				memcpy(outToken, token, TOKEN_SIZE);
				SecureZeroMemory(token, sizeof(token));
				return true;
			}

			SecureZeroMemory(token, sizeof(token));
			return false;
		}

		bool IdentityTokenManager::VerifyToken(PlayerUID xuid, const uint8_t token[TOKEN_SIZE]) const
		{
			EnterCriticalSection(&m_lock);
			auto it = m_tokens.find(xuid);
			if (it == m_tokens.end() || it->second.size() != TOKEN_SIZE)
			{
				LeaveCriticalSection(&m_lock);
				return false;
			}

			// Constant-time comparison to prevent timing attacks
			uint8_t diff = 0;
			for (int i = 0; i < TOKEN_SIZE; ++i)
			{
				diff |= it->second[i] ^ token[i];
			}
			LeaveCriticalSection(&m_lock);
			return diff == 0;
		}

		bool IdentityTokenManager::RevokeToken(PlayerUID xuid)
		{
			EnterCriticalSection(&m_lock);
			auto it = m_tokens.find(xuid);
			if (it == m_tokens.end())
			{
				LeaveCriticalSection(&m_lock);
				return false;
			}
			SecureZeroMemory(it->second.data(), it->second.size());
			m_tokens.erase(it);
			bool saved = Save();
			LeaveCriticalSection(&m_lock);
			return saved;
		}

		bool IdentityTokenManager::Load()
		{
			std::string text;
			if (!FileUtils::ReadTextFile(m_filePath, &text))
			{
				return false;
			}

			if (text.empty())
			{
				return true;
			}

			OrderedJson root;
			try
			{
				root = OrderedJson::parse(StringUtils::StripUtf8Bom(text));
			}
			catch (const nlohmann::json::exception &)
			{
				LogErrorf("security", "failed to parse %s", m_filePath.c_str());
				return false;
			}

			if (!root.is_object() || !root.contains("tokens") || !root["tokens"].is_object())
			{
				return true;
			}

			for (auto it = root["tokens"].begin(); it != root["tokens"].end(); ++it)
			{
				const std::string &xuidStr = it.key();
				if (!it.value().is_string()) continue;

				unsigned long long parsed = 0;
				if (!StringUtils::TryParseUnsignedLongLong(xuidStr, &parsed) || parsed == 0ULL)
					continue;

				std::vector<uint8_t> tokenBytes;
				if (!Base64ToBytes(it.value().get<std::string>(), tokenBytes))
					continue;
				if (tokenBytes.size() != TOKEN_SIZE)
					continue;

				m_tokens[static_cast<PlayerUID>(parsed)] = tokenBytes;
			}

			return true;
		}

		bool IdentityTokenManager::Save() const
		{
			OrderedJson root = OrderedJson::object();
			OrderedJson tokens = OrderedJson::object();

			for (const auto &pair : m_tokens)
			{
				std::string xuidStr = FormatXuid(pair.first);
				std::string tokenB64 = BytesToBase64(pair.second.data(), TOKEN_SIZE);
				tokens[xuidStr] = tokenB64;
			}

			root["tokens"] = tokens;

			std::string json = root.dump(2) + "\n";
			if (!FileUtils::WriteTextFileAtomic(m_filePath, json))
			{
				LogErrorf("security", "failed to write %s", m_filePath.c_str());
				return false;
			}
			return true;
		}

		IdentityTokenManager &GetIdentityTokenManager()
		{
			static IdentityTokenManager s_instance;
			return s_instance;
		}
	}
}

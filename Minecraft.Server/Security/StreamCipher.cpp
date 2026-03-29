#include "stdafx.h"
#include "StreamCipher.h"

#ifdef _WINDOWS64
#include <bcrypt.h>
#pragma comment(lib, "bcrypt.lib")
#endif

#include <cstring>

namespace ServerRuntime
{
	namespace Security
	{
		StreamCipher::StreamCipher()
			: m_sendKeystreamPos(AES_BLOCK)
			, m_recvKeystreamPos(AES_BLOCK)
			, m_active(false)
		{
#ifdef _WINDOWS64
			m_hAlg = nullptr;
			m_hKey = nullptr;
#endif
			memset(m_sendCounter, 0, sizeof(m_sendCounter));
			memset(m_recvCounter, 0, sizeof(m_recvCounter));
			memset(m_sendKeystream, 0, sizeof(m_sendKeystream));
			memset(m_recvKeystream, 0, sizeof(m_recvKeystream));
		}

		StreamCipher::~StreamCipher()
		{
			Reset();
		}

		void StreamCipher::Initialize(const uint8_t key[KEY_SIZE], Role role)
		{
			if (m_active)
			{
				Reset();
			}

#ifdef _WINDOWS64
			NTSTATUS status;

			status = BCryptOpenAlgorithmProvider(&m_hAlg, BCRYPT_AES_ALGORITHM, nullptr, 0);
			if (!BCRYPT_SUCCESS(status))
			{
				m_hAlg = nullptr;
				return;
			}

			// Set ECB mode -- we manage CTR ourselves for streaming support
			status = BCryptSetProperty(m_hAlg, BCRYPT_CHAINING_MODE,
				(PUCHAR)BCRYPT_CHAIN_MODE_ECB, sizeof(BCRYPT_CHAIN_MODE_ECB), 0);
			if (!BCRYPT_SUCCESS(status))
			{
				BCryptCloseAlgorithmProvider(m_hAlg, 0);
				m_hAlg = nullptr;
				return;
			}

			// Create symmetric key from first 16 bytes
			status = BCryptGenerateSymmetricKey(m_hAlg, &m_hKey, nullptr, 0,
				(PUCHAR)key, AES_BLOCK, 0);
			if (!BCRYPT_SUCCESS(status))
			{
				BCryptCloseAlgorithmProvider(m_hAlg, 0);
				m_hAlg = nullptr;
				m_hKey = nullptr;
				return;
			}

			// Derive separate counters for send and recv to prevent CTR nonce reuse.
			// Flipping the top bit of byte 0 guarantees the two counter spaces never
			// overlap (one in 0x00-0x7F range, the other in 0x80-0xFF for byte 0).
			// Server send = IV,        Server recv = IV^0x80
			// Client send = IV^0x80,   Client recv = IV
			// This ensures: server-send matches client-recv, client-send matches server-recv.
			uint8_t ivBase[AES_BLOCK];
			uint8_t ivFlipped[AES_BLOCK];
			memcpy(ivBase, key + AES_BLOCK, AES_BLOCK);
			memcpy(ivFlipped, key + AES_BLOCK, AES_BLOCK);
			ivFlipped[0] ^= 0x80;

			if (role == Server)
			{
				memcpy(m_sendCounter, ivBase, AES_BLOCK);
				memcpy(m_recvCounter, ivFlipped, AES_BLOCK);
			}
			else
			{
				memcpy(m_sendCounter, ivFlipped, AES_BLOCK);
				memcpy(m_recvCounter, ivBase, AES_BLOCK);
			}

			SecureZeroMemory(ivBase, sizeof(ivBase));
			SecureZeroMemory(ivFlipped, sizeof(ivFlipped));

			m_sendKeystreamPos = AES_BLOCK;  // force generation on first use
			m_recvKeystreamPos = AES_BLOCK;
			m_active = true;
#endif
		}

		void StreamCipher::Reset()
		{
#ifdef _WINDOWS64
			if (m_hKey != nullptr)
			{
				BCryptDestroyKey(m_hKey);
				m_hKey = nullptr;
			}
			if (m_hAlg != nullptr)
			{
				BCryptCloseAlgorithmProvider(m_hAlg, 0);
				m_hAlg = nullptr;
			}
#endif
			SecureZeroMemory(m_sendCounter, sizeof(m_sendCounter));
			SecureZeroMemory(m_recvCounter, sizeof(m_recvCounter));
			SecureZeroMemory(m_sendKeystream, sizeof(m_sendKeystream));
			SecureZeroMemory(m_recvKeystream, sizeof(m_recvKeystream));
			m_sendKeystreamPos = AES_BLOCK;
			m_recvKeystreamPos = AES_BLOCK;
			m_active = false;
		}

		void StreamCipher::IncrementCounter(uint8_t counter[AES_BLOCK])
		{
			// Big-endian 128-bit increment (standard NIST CTR convention)
			for (int i = AES_BLOCK - 1; i >= 0; --i)
			{
				if (++counter[i] != 0)
					break;
			}
		}

		void StreamCipher::GenerateKeystream(uint8_t counter[AES_BLOCK], uint8_t keystream[AES_BLOCK])
		{
#ifdef _WINDOWS64
			ULONG cbResult = 0;
			NTSTATUS status = BCryptEncrypt(m_hKey, counter, AES_BLOCK, nullptr,
				nullptr, 0, keystream, AES_BLOCK, &cbResult, 0);  // flags=0: exact block, no padding
			if (!BCRYPT_SUCCESS(status))
			{
				SecureZeroMemory(keystream, AES_BLOCK);
				m_active = false;
				return;
			}
			IncrementCounter(counter);
#endif
		}

		void StreamCipher::Encrypt(uint8_t *data, int length)
		{
			if (!m_active || data == nullptr || length <= 0)
			{
				return;
			}

			for (int i = 0; i < length; ++i)
			{
				if (m_sendKeystreamPos >= AES_BLOCK)
				{
					GenerateKeystream(m_sendCounter, m_sendKeystream);
					m_sendKeystreamPos = 0;
				}
				data[i] ^= m_sendKeystream[m_sendKeystreamPos++];
			}
		}

		void StreamCipher::Decrypt(uint8_t *data, int length)
		{
			if (!m_active || data == nullptr || length <= 0)
			{
				return;
			}

			for (int i = 0; i < length; ++i)
			{
				if (m_recvKeystreamPos >= AES_BLOCK)
				{
					GenerateKeystream(m_recvCounter, m_recvKeystream);
					m_recvKeystreamPos = 0;
				}
				data[i] ^= m_recvKeystream[m_recvKeystreamPos++];
			}
		}

		bool StreamCipher::GenerateKey(uint8_t outKey[KEY_SIZE])
		{
#ifdef _WINDOWS64
			NTSTATUS status = BCryptGenRandom(nullptr, outKey, KEY_SIZE,
				BCRYPT_USE_SYSTEM_PREFERRED_RNG);
			return BCRYPT_SUCCESS(status);
#else
			// Fallback: not cryptographically random
			for (int i = 0; i < KEY_SIZE; ++i)
			{
				outKey[i] = static_cast<uint8_t>(rand() & 0xFF);
			}
			return true;
#endif
		}
	}
}

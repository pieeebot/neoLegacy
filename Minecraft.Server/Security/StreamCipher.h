#pragma once

#include <cstdint>

#ifdef _WINDOWS64
#include <Windows.h>
#include <bcrypt.h>
#endif

namespace ServerRuntime
{
	namespace Security
	{
		/**
		 * AES-128-CTR stream cipher for game traffic encryption.
		 *
		 * Uses the Windows BCrypt API to generate AES-encrypted keystream
		 * blocks that are XOR'd with plaintext. Each direction (send/recv)
		 * maintains its own counter for independent keystream generation.
		 *
		 * Key material: 32 bytes (16-byte AES key + 16-byte IV/nonce).
		 * The IV is used as the initial counter block for both directions.
		 *
		 * Usage:
		 * 1. Server generates a random 32-byte key via GenerateKey()
		 * 2. Key is sent to the client in the MC|CKey CustomPayloadPacket
		 * 3. Both sides call Initialize() with the same 32 bytes
		 * 4. All subsequent TCP traffic is encrypted via Encrypt/Decrypt
		 */
		class StreamCipher
		{
		public:
			static const int KEY_SIZE = 32;  // 16 AES key + 16 IV

			enum Role { Server, Client };

			StreamCipher();
			~StreamCipher();

			StreamCipher(const StreamCipher &) = delete;
			StreamCipher &operator=(const StreamCipher &) = delete;
			StreamCipher(StreamCipher &&) = delete;
			StreamCipher &operator=(StreamCipher &&) = delete;

			/**
			 * Initialize with key material. First 16 bytes = AES key, last 16 bytes = IV.
			 * Role determines counter assignment to prevent nonce reuse between directions:
			 *   Server: send=IV, recv=IV^0x80 (top bit flipped)
			 *   Client: send=IV^0x80, recv=IV
			 * This ensures server-send matches client-recv and vice versa.
			 */
			void Initialize(const uint8_t key[KEY_SIZE], Role role = Server);

			/**
			 * AES-CTR encrypt data in place for sending.
			 */
			void Encrypt(uint8_t *data, int length);

			/**
			 * AES-CTR decrypt data in place after receiving.
			 */
			void Decrypt(uint8_t *data, int length);

			/**
			 * Returns true if the cipher has been initialized.
			 */
			bool IsActive() const { return m_active; }

			/**
			 * Reset to inactive state and securely wipe all key material.
			 */
			void Reset();

			/**
			 * Generate 32 cryptographically random bytes (16 AES key + 16 IV).
			 */
			static bool GenerateKey(uint8_t outKey[KEY_SIZE]);

		private:
			static const int AES_BLOCK = 16;

			static void IncrementCounter(uint8_t counter[AES_BLOCK]);
			void GenerateKeystream(uint8_t counter[AES_BLOCK], uint8_t keystream[AES_BLOCK]);

#ifdef _WINDOWS64
			BCRYPT_ALG_HANDLE m_hAlg;
			BCRYPT_KEY_HANDLE m_hKey;
#endif
			uint8_t m_sendCounter[AES_BLOCK];
			uint8_t m_recvCounter[AES_BLOCK];
			uint8_t m_sendKeystream[AES_BLOCK];
			uint8_t m_recvKeystream[AES_BLOCK];
			int m_sendKeystreamPos;
			int m_recvKeystreamPos;
			bool m_active;
		};
	}
}

#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

inline constexpr std::uint8_t MU_CRYPTO_VERSION_AES256GCM = 0x01;

// Saved logins are encrypted for this user on this machine. Each target compiles one adapter from Adapters/; one
// without a credential store reports false here and fails every call below.
bool mu_credentials_available();
bool mu_encrypt_blob(const void* input, std::size_t inputSize, std::vector<std::uint8_t>& output);
bool mu_decrypt_blob(const void* input, std::size_t inputSize, std::vector<std::uint8_t>& output);
// A blob saved by builds before MU_CRYPTO_VERSION_AES256GCM (Windows DPAPI, or its stand-in elsewhere).
bool mu_decrypt_legacy_blob(const void* input, std::size_t inputSize, std::vector<std::uint8_t>& output);

// Cryptographically secure random bytes; false when the target has none.
bool mu_random_bytes(void* output, std::size_t size);
inline constexpr std::size_t MU_SHA256_SIZE = 32;
// The SHA-256 of the input into output[MU_SHA256_SIZE]; false when the target has no hash.
bool mu_sha256(const void* input, std::size_t inputSize, std::uint8_t* output);

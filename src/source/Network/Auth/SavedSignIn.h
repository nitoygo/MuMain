#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <vector>

namespace Network::Auth
{
// A site sign-in the game keeps for "Continue as <name>": the account's name and the site's refresh token.
struct SavedSignIn
{
    std::string name;
    std::string refreshToken;
};

// Keeps a SavedSignIn encrypted (the saved logins' blob, PlatformCrypto.h), as hex in a setting the caller names
// through read and write. Never in plain text.
class SavedSignInStore
{
public:
    using BlobCipher = bool (*)(const void* input, std::size_t inputSize, std::vector<std::uint8_t>& output);

    SavedSignInStore(BlobCipher encrypt, BlobCipher decrypt, std::function<std::wstring()> read,
                     std::function<void(const std::wstring&)> write);

    // Nullopt when nothing is saved or it no longer decrypts (another user or machine).
    std::optional<SavedSignIn> Load() const;
    // False, and nothing written, when it can't be encrypted.
    bool Save(const SavedSignIn& signIn);
    void Clear();

private:
    BlobCipher m_encrypt;
    BlobCipher m_decrypt;
    std::function<std::wstring()> m_read;
    std::function<void(const std::wstring&)> m_write;
};
} // namespace Network::Auth

#include "stdafx.h"

#include "Network/Auth/SavedSignIn.h"

#include "Data/GameConfig/GameConfig.h"

#include <utility>

namespace Network::Auth
{
namespace
{
// Neither a login name nor a base64url token holds a line break.
constexpr char kSeparator = '\n';
} // namespace

SavedSignInStore::SavedSignInStore(BlobCipher encrypt, BlobCipher decrypt, std::function<std::wstring()> read,
                                   std::function<void(const std::wstring&)> write)
    : m_encrypt(encrypt), m_decrypt(decrypt), m_read(std::move(read)), m_write(std::move(write))
{
}

std::optional<SavedSignIn> SavedSignInStore::Load() const
{
    const std::vector<BYTE> sealed = GameConfig::HexToBinary(m_read());
    std::vector<std::uint8_t> plain;
    if (sealed.empty() || !m_decrypt(sealed.data(), sealed.size(), plain))
        return std::nullopt;

    const std::string text(plain.begin(), plain.end());
    const std::size_t separator = text.find(kSeparator);
    if (separator == std::string::npos || separator + 1 == text.size())
        return std::nullopt;
    return SavedSignIn{.name = text.substr(0, separator), .refreshToken = text.substr(separator + 1)};
}

bool SavedSignInStore::Save(const SavedSignIn& signIn)
{
    const std::string plain = signIn.name + kSeparator + signIn.refreshToken;
    std::vector<std::uint8_t> sealed;
    if (signIn.refreshToken.empty() || !m_encrypt(plain.data(), plain.size(), sealed))
        return false;
    m_write(GameConfig::BinaryToHex(sealed.data(), static_cast<DWORD>(sealed.size())));
    return true;
}

void SavedSignInStore::Clear()
{
    m_write(L"");
}
} // namespace Network::Auth

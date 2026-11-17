#include "stdafx.h"

#include "UI/Windows/LoginProviderButtons.h"

#include <RmlUi/Core/DataModelHandle.h>

namespace UI::Login
{
void BindProviderButtons(Rml::DataModelConstructor& constructor, std::vector<Network::Auth::LoginProvider>& providers)
{
    auto provider = constructor.RegisterStruct<Network::Auth::LoginProvider>();
    provider.RegisterMember("id", &Network::Auth::LoginProvider::id);
    provider.RegisterMember("display_name", &Network::Auth::LoginProvider::displayName);
    constructor.RegisterArray<std::vector<Network::Auth::LoginProvider>>();
    constructor.Bind("providers", &providers);
}
} // namespace UI::Login

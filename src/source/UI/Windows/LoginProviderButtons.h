// The login window's "sign in with <provider>" buttons, one per provider of the target's ticket source.
#pragma once

#include <vector>

#include "Network/Auth/LoginTicketSource.h"

namespace Rml
{
class DataModelConstructor;
}

namespace UI::Login
{
// Binds "providers" (id, display_name) for the login document's data-for; an empty list hides the buttons.
void BindProviderButtons(Rml::DataModelConstructor& constructor, std::vector<Network::Auth::LoginProvider>& providers);
} // namespace UI::Login

#include "stdafx.h"
#include <doctest.h>

#include "RmlLayoutFixture.h"
#include "UI/Windows/LoginProviderButtons.h"

#include <string>
#include <vector>

namespace
{
using UI::Tests::AssetMarkup;
using UI::Tests::RmlLayoutFixture;

// The login document of one theme over a model with just the provider bindings, as LoginWin binds them.
class LoginProviderFixture
{
public:
    explicit LoginProviderFixture(const std::string& theme)
    {
        m_Layout.Configure({1280, 720}, 100);
        Rml::DataModelConstructor constructor = m_Layout.context->CreateDataModel("login");
        REQUIRE(constructor);
        UI::Login::BindProviderButtons(constructor, providers);
        constructor.Bind("sign_in_error", &signInError);
        m_Handle = constructor.GetModelHandle();
        m_Document = m_Layout.LoadMarkup(AssetMarkup(theme, "login", /*keepBindings=*/true));
    }

    void SetProviders(std::vector<Network::Auth::LoginProvider> list)
    {
        providers = std::move(list);
        m_Handle.DirtyVariable("providers");
        m_Layout.Refresh();
    }

    // The buttons laid out on screen, by their text.
    std::vector<std::string> ShownButtons() const
    {
        Rml::ElementList buttons;
        m_Document->QuerySelectorAll(buttons, ".login-provider-btn");
        std::vector<std::string> shown;
        for (Rml::Element* button : buttons)
        {
            if (button->GetOffsetWidth() > 0 && button->GetOffsetHeight() > 0)
                shown.push_back(button->GetInnerRML());
        }
        return shown;
    }

    std::vector<Network::Auth::LoginProvider> providers;
    Rml::String signInError;

private:
    RmlLayoutFixture m_Layout;
    Rml::DataModelHandle m_Handle;
    Rml::ElementDocument* m_Document = nullptr;
};
} // namespace

TEST_CASE("The login window shows a button per sign-in provider and none without any [ui][login]")
{
    for (const char* theme : {"legacy", "modern"})
    {
        CAPTURE(theme);
        LoginProviderFixture login(theme);
        CHECK(login.ShownButtons().empty());

        login.SetProviders({{"discord", "Discord"}, {"google", "Google"}});
        CHECK(login.ShownButtons() == std::vector<std::string>{"Discord", "Google"});

        login.SetProviders({});
        CHECK(login.ShownButtons().empty());
    }
}

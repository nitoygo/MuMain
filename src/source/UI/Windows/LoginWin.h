//*****************************************************************************
// File: LoginWin.h
//*****************************************************************************
#pragma once

#include "UI/Core/WindowObject.h"

#include "UI/RmlBridge/RmlThemedView.h"

#include "Network/Auth/LoginRequest.h"

#include <vector>

namespace Rml { class ElementDocument; }

// The login dialog: RmlUi (login.rml) renders everything, username/password entry included -- the
// two fields are stock <input> elements (#input_account/#input_password) bound to
// LoginRmlModel::username/password. External error-recovery code asks for focus through
// FocusUsername()/FocusPassword() rather than reaching in for a widget pointer.
class CLoginWin : public mu::ui::window::CObject
{
protected:
    bool m_bRememberMeChecked = false;
    bool m_bSavePasswordChecked = false;

    // Snapshot used to detect that the player edited the username/password, so stored
    // credentials can be dropped.
    wchar_t m_prevUsername[MAX_USERNAME_SIZE + 1] = {};
    wchar_t m_prevPassword[MAX_PASSWORD_SIZE + 1] = {};

    // This window's own bounding box (no shared rect facility on the CObject side).
    POINT m_ptPos = {};
    SIZE m_Size = {};

public:
    CLoginWin();
    ~CLoginWin() override;
    void Create();
    void Release();
    void SetPosition(int nXCoord, int nYCoord);
    void Show(bool bShow) override;

    // Sends a login without touching the text boxes or the remembered
    // credentials in config.ini. The manual path calls it after validating
    // what the player typed; automation calls it directly.
    void SubmitCredentials(const wchar_t* pszUsername, const wchar_t* pszPassword);
    // The one send path for every login: a password or a ticket from a sign-in provider.
    void SubmitLoginRequest(const Network::Auth::LoginRequest& request);

    // Error-recovery focus redirection (WSclient.cpp on connect, MsgWin.cpp per login-failure
    // reason). selectAll reproduces the native GiveFocus(TRUE) that pre-selected the text so a
    // retry overwrites it in one keystroke.
    void FocusUsername(bool selectAll = false);
    void FocusPassword(bool selectAll = false);
    int GetWidth() const
    {
        return m_Size.cx;
    }
    void RefreshLayout();
    int GetHeight() const
    {
        return m_Size.cy;
    }

    // Bound to the RmlUi login document's click callbacks. Act immediately rather than setting a
    // flag for UpdateWhileActive() to consume later: this fires from the SDL event pump, always
    // before CSceneUICoordinator::Update() runs the same frame. Each re-checks the "remember
    // password" prompt's Pending state directly since these run before UpdateWhileShown()'s Tick().
    void RmlClickOk();
    void RmlClickCancel();
    void RmlToggleRememberMe();
    void RmlToggleSavePassword();
    // A provider's sign-in button: asks the ticket source for a ticket, then logs in with it.
    void RmlClickProvider(const std::string& providerId);
    // Signs in with a provider the player already chose outside the game, once per run. False when there is none.
    bool ContinueWithChosenProvider();
    bool IsSignInPending() const
    {
        return m_signInPending;
    }

    // mu::ui::window::IObject
    bool Render() override;
    // Rebuilds this window's RmlUi document/model for the active theme. No-op if never opened.
    // Claims clicks within its own bounding box only -- not modal, world/credits/system-menu stay
    // reachable around it. Depth 20.0f is below the full-screen-claiming overlays (CSysMenuWin/
    // CMsgWin/CCreditWin), so no explicit modal check is needed here.
    bool UpdateMouseEvent() override;
    bool UpdateKeyEvent() override
    {
        return true;
    }
    float GetLayerDepth() override
    {
        return 20.0f;
    }

protected:
    // Runs each enabled frame, including while hidden. Keeps the Remember-Password prompt ticking;
    // login input is active only while this window is visible and no higher-depth modal owns it.
    bool UpdateWhileShown() override;
    // Runs only while also active -- OK/Cancel/Enter/Esc submit handling.
    bool UpdateWhileActive() override;

    void RequestLogin();
    void CancelLogin();

    // "Remember me" credential handling, split out of the update loop.
    void ApplyRememberPasswordChoice();
    void RevokeSavedCredentialsIfEdited();

private:
    int FirstLoad = 0;
    // A provider sign-in is running (the player may be in a browser); further clicks wait for it.
    bool m_signInPending = false;
    // Only the first login window continues on its own; after a failure or a logout the player picks again.
    bool m_autoContinueTried = false;

    // Shared by the immediate RmlUi callbacks and UpdateWhileActive()'s keyboard polling; each
    // re-checks the "remember password" prompt's live Pending state itself.
    void SubmitLogin();
    void SubmitCancel();
    // Plays the click sound of the button a key stands for (a selector of the login document).
    void PlayKeyClick(const char* button);
    void ApplyRememberMeChange();
    void ApplySavePasswordChange();
    void OnProviderTicket(const Network::Auth::LoginTicketResult& result);
    void SetSignInError(const wchar_t* text);

    struct LoginRmlModel
    {
        bool rememberMeChecked = false;
        bool savePasswordChecked = false;
        Rml::String serverName;
        // Synced from I18N::Game::* slots to keep these localization-correct.
        Rml::String accountLabel;
        Rml::String passwordLabel;
        Rml::String rememberMeLabel;
        Rml::String savePasswordLabel;
        Rml::String trustWarning;
        Rml::String okLabel;
        Rml::String cancelLabel;
        Rml::String signInLabel;
        Rml::String signInError;

        // The target's sign-in providers; none hides their buttons.
        std::vector<Network::Auth::LoginProvider> providers;

        // Two-way bound to #input_account/#input_password. RmlUi owns the edit buffers, caret,
        // selection, masking and IME; these carry the committed values that SubmitCredentials()
        // and the edit-detection snapshot read.
        Rml::String username;
        Rml::String password;
    };
    UI::RmlBridge::ThemedView<LoginRmlModel> m_RmlView{"login",
        [this](Rml::DataModelConstructor& c, LoginRmlModel& model) { BindRmlModel(c, model); },
        {{"Data/Interface/RmlUi/login.rml"}},
        {.afterBuild = [this] { ApplyCredentialLimits(); }, .afterReload = [this] { OnRmlReloaded(); }}};

    void SyncRmlModel();
    void BindRmlModel(Rml::DataModelConstructor& c, LoginRmlModel& model);
    void OnRmlReloaded();
    // Pushes MAX_USERNAME_SIZE/MAX_PASSWORD_SIZE onto the two fields, so the caps stay one C++ rule
    // rather than a literal duplicated per theme.
    void ApplyCredentialLimits();
    // Shared by FocusUsername()/FocusPassword().
    void FocusCredentialField(const char* elementId, bool selectAll);
};

extern CLoginWin g_LoginWin;

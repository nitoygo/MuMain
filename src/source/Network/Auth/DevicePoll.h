#pragma once

#include "Network/Auth/SiteSignIn.h"

namespace Network::Auth
{
// RFC 8628's polling for a device code: keep asking at the site's interval while the player hasn't decided, slow
// down when told to, stop on a sign-in, a refusal or the code's expiry.
class DevicePoll
{
public:
    enum class Step
    {
        Wait,
        SignedIn,
        Failed,
    };

    // A slow_down adds this to the interval (RFC 8628 3.5).
    static constexpr int SlowDownSeconds = 5;

    DevicePoll(int intervalSeconds, int expiresInSeconds);

    // What to do after the answer to one poll.
    Step OnAnswer(const SiteSignIn::TokenAnswer& answer);

    int IntervalSeconds() const
    {
        return m_intervalSeconds;
    }

    bool Expired(int elapsedSeconds) const
    {
        return elapsedSeconds >= m_expiresInSeconds;
    }

private:
    int m_intervalSeconds;
    int m_expiresInSeconds;
};
} // namespace Network::Auth

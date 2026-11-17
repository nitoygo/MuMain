#include "stdafx.h"

#include "Network/Auth/DevicePoll.h"

#include <algorithm>

namespace Network::Auth
{
DevicePoll::DevicePoll(int intervalSeconds, int expiresInSeconds)
    : m_intervalSeconds(std::max(intervalSeconds, 1)), m_expiresInSeconds(expiresInSeconds)
{
}

DevicePoll::Step DevicePoll::OnAnswer(const SiteSignIn::TokenAnswer& answer)
{
    if (answer.Succeeded())
        return Step::SignedIn;
    if (answer.error == "slow_down")
    {
        m_intervalSeconds += SlowDownSeconds;
        return Step::Wait;
    }
    // A dropped connection or a busy site is asked again at the next interval, until the code expires.
    const bool keepWaiting = answer.error == "authorization_pending" || answer.error == SiteSignIn::NetworkError ||
                             answer.error == "temporarily_unavailable";
    return keepWaiting ? Step::Wait : Step::Failed;
}
} // namespace Network::Auth

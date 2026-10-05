#pragma once
#include "CoreLogic.hpp"
#include <optional>
#include <string>

namespace du::sync {

struct RemoteProfile {
    std::string displayName;
    bool displayFirst = true;
    bool displayEnabled = true;
    Visibility displayVisibility = Visibility::All;
    Visibility stickerVisibility = Visibility::All;
    bool displayAllowed = false;
    bool stickersAllowed = false;
};

inline std::optional<RemoteProfile> getCached(int) { return std::nullopt; }
inline void publishLocal() {}

// Production TODO:
// - use User Data API or an authenticated service for cross-player names;
// - send custom names as PENDING and distribute them only after server-side moderation;
// - never rely on the client moderation check for public safety;
// - enforce ALL / FRIENDS / ME on the server, not only in the client;
// - never trust a client-supplied Account ID for owner or economy operations.

} // namespace du::sync

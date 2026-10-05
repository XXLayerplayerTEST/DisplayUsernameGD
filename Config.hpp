#pragma once

namespace du::config {

// Prototype owner identity. For public release this must be replaced with
// server-side authentication. Account ID can stay 0 for local testing;
// R panel is restricted to the real owner Account ID.
constexpr int kOwnerAccountID = 40224020;
constexpr char kOwnerUsername[] = "XXLayerPlayer";

// v0.3.x uses local prototype storage so the UI can be tested without a server.
constexpr bool kUseLocalPrototypeBackend = true;

// Stickers stay disabled until the official 2.209 sticker system is actually
// available and the mod has been updated for that GD / Geode version.
constexpr bool kStickersAvailable = false;

} // namespace du::config

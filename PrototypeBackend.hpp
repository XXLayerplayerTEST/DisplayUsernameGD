#pragma once

#include <Geode/Geode.hpp>
#include "Models.hpp"
#include <optional>
#include <string>
#include <vector>

namespace du::backend {

// Local prototype backend. It is intentionally NOT secure and exists so the
// complete UI/economy/rate flow can be tested. A public multi-user release
// still needs an authenticated server for balances, private names and owner rates.

EconomyState economy();
void setEconomy(EconomyState const& value);

bool spendDiamonds(int amount);
void addDiamonds(int amount);

bool buyNameExpansionKey();
bool redeemNameExpansionKey();

bool buyStickerAccessKey();
bool buyStickerKey();

bool isOwner();

// Owner moderation prototype. These values are stored locally for UI testing.
// Public multi-user enforcement must be performed by an authenticated server.
bool banUser(int accountID, int durationSeconds, bool permanent);
bool unbanUser(int accountID);
bool isUserBanned(int accountID);
bool isUserPermanentlyBanned(int accountID);
int userBanRemainingSeconds(int accountID);

bool setUserDisplayOverride(int accountID, std::string const& displayName);
bool clearUserDisplayOverride(int accountID);
std::optional<std::string> userDisplayOverride(int accountID);

bool rateLevel(RatedLevel const& entry);
bool removeDaily(int levelID);
bool removeRate(int levelID);
std::vector<RatedLevel> ratedLevels();
std::optional<RatedLevel> dailyLevel();
std::optional<RatedLevel> ratedLevel(int levelID);

bool isBaseClaimed(int levelID);
bool isDailyClaimed(int dailyGeneration);

ClaimResult claimCompletion(int levelID);

} // namespace du::backend

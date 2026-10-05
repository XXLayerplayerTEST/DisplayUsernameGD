#include "PrototypeBackend.hpp"
#include "Config.hpp"
#include "DisplayState.hpp"
#include <algorithm>
#include <cctype>
#include <string>
#include <ctime>
#include <limits>

using namespace geode::prelude;

namespace du::backend {
namespace {


matjson::Value toJSON(RatedLevel const& e) {
    matjson::Value v = matjson::Value::object();
    v["levelID"] = e.levelID;
    v["levelName"] = e.levelName;
    v["creatorName"] = e.creatorName;
    v["difficulty"] = static_cast<int>(e.difficulty);
    v["demonType"] = static_cast<int>(e.demonType);
    v["feature"] = static_cast<int>(e.feature);
    v["reward"] = e.reward;
    v["daily"] = e.daily;
    v["dailyBonus"] = e.dailyBonus;
    v["dailyGeneration"] = e.dailyGeneration;
    return v;
}

RatedLevel fromJSON(matjson::Value const& v) {
    RatedLevel e;
    e.levelID = v["levelID"].asInt().unwrapOr(0);
    e.levelName = v["levelName"].asString().unwrapOr("");
    e.creatorName = v["creatorName"].asString().unwrapOr("");
    e.difficulty = static_cast<Difficulty>(v["difficulty"].asInt().unwrapOr(0));
    e.demonType = static_cast<DemonType>(v["demonType"].asInt().unwrapOr(0));
    e.feature = static_cast<FeatureTier>(v["feature"].asInt().unwrapOr(0));
    e.reward = v["reward"].asInt().unwrapOr(0);
    e.daily = v["daily"].asBool().unwrapOr(false);
    e.dailyBonus = v["dailyBonus"].asInt().unwrapOr(0);
    e.dailyGeneration = v["dailyGeneration"].asInt().unwrapOr(0);
    return e;
}

std::vector<RatedLevel> loadLevels() {
    std::vector<RatedLevel> out;
    auto raw = Mod::get()->getSavedValue<matjson::Value>("prototype-rated-levels", matjson::Value::array());
    if (!raw.isArray()) return out;
    for (auto const& item : raw) {
        auto e = fromJSON(item);
        if (e.levelID > 0 && validReward(e.reward) && validDailyBonus(e.dailyBonus)) out.push_back(e);
    }
    return out;
}

void saveLevels(std::vector<RatedLevel> const& levels) {
    matjson::Value raw = matjson::Value::array();
    for (auto const& e : levels) raw.push(toJSON(e));
    Mod::get()->setSavedValue("prototype-rated-levels", raw);
}

bool claimedBaseImpl(int levelID) {
    return Mod::get()->getSavedValue<bool>(fmt::format("prototype-claim-base-{}", levelID), false);
}
void setClaimedBase(int levelID) {
    Mod::get()->setSavedValue(fmt::format("prototype-claim-base-{}", levelID), true);
}

bool claimedDailyImpl(int generation) {
    if (generation <= 0) return false;
    return Mod::get()->getSavedValue<bool>(fmt::format("prototype-claim-daily-{}", generation), false);
}
void setClaimedDaily(int generation) {
    if (generation > 0) Mod::get()->setSavedValue(fmt::format("prototype-claim-daily-{}", generation), true);
}

} // namespace

EconomyState economy() {
    EconomyState e;
    e.diamonds = std::clamp(Mod::get()->getSavedValue<int>("diamonds", 0), -100000000, 100000000);
    e.nameExpansionKeys = std::max(0, Mod::get()->getSavedValue<int>("name-expansion-keys", 0));
    e.nameExpanded = DisplayState::nameExpanded();
    e.stickerAccess = Mod::get()->getSavedValue<bool>("sticker-access", false);
    e.stickerKeys = std::max(0, Mod::get()->getSavedValue<int>("sticker-keys", 0));
    e.stickerSlots = std::clamp(Mod::get()->getSavedValue<int>("sticker-slots", 0), 0, kMaxStickerSlots);
    return e;
}

void setEconomy(EconomyState const& e) {
    Mod::get()->setSavedValue("diamonds", std::clamp(e.diamonds, -100000000, 100000000));
    Mod::get()->setSavedValue("name-expansion-keys", std::max(0, e.nameExpansionKeys));
    DisplayState::setNameExpanded(e.nameExpanded);
    Mod::get()->setSavedValue("sticker-access", e.stickerAccess);
    Mod::get()->setSavedValue("sticker-keys", std::max(0, e.stickerKeys));
    Mod::get()->setSavedValue("sticker-slots", std::clamp(e.stickerSlots, 0, kMaxStickerSlots));
}

bool spendDiamonds(int amount) {
    if (amount < 0) return false;
    auto e = economy();
    if (e.diamonds < amount) return false;
    e.diamonds -= amount;
    setEconomy(e);
    return true;
}

void addDiamonds(int amount) {
    if (amount <= 0) return;
    auto e = economy();
    e.diamonds = std::min(100000000, e.diamonds + amount);
    setEconomy(e);
}

bool buyNameExpansionKey() {
    auto e = economy();
    if (e.nameExpanded || e.diamonds < kNameExpansionPrice) return false;
    e.diamonds -= kNameExpansionPrice;
    e.nameExpansionKeys = 0;
    e.nameExpanded = true;
    setEconomy(e);
    return true;
}

bool redeemNameExpansionKey() {
    auto e = economy();
    if (e.nameExpanded || e.nameExpansionKeys <= 0) return false;
    e.nameExpansionKeys -= 1;
    e.nameExpanded = true;
    setEconomy(e);
    return true;
}

bool buyStickerAccessKey() {
    if (!config::kStickersAvailable) return false;
    auto e = economy();
    if (e.stickerAccess || e.diamonds < kStickerAccessPrice) return false;
    e.diamonds -= kStickerAccessPrice;
    e.stickerAccess = true;
    e.stickerSlots = std::max(1, e.stickerSlots);
    setEconomy(e);
    return true;
}

bool buyStickerKey() {
    if (!config::kStickersAvailable) return false;
    auto e = economy();
    if (!e.stickerAccess || e.diamonds < kStickerKeyPrice) return false;
    e.diamonds -= kStickerKeyPrice;
    e.stickerKeys += 1;
    setEconomy(e);
    return true;
}

bool isOwner() {
    auto account = GJAccountManager::sharedState();
    return config::kOwnerAccountID > 0 && account && account->m_accountID == config::kOwnerAccountID;
}

namespace {
std::string banUntilKey(int accountID) { return fmt::format("prototype-user-ban-until-{}", accountID); }
std::string banPermanentKey(int accountID) { return fmt::format("prototype-user-ban-permanent-{}", accountID); }
std::string displayOverrideKey(int accountID) { return fmt::format("prototype-user-display-override-{}", accountID); }
}

bool banUser(int accountID, int durationSeconds, bool permanent) {
    if (!isOwner() || accountID <= 0 || accountID == config::kOwnerAccountID) return false;
    if (!permanent && durationSeconds <= 0) return false;

    Mod::get()->setSavedValue(banPermanentKey(accountID), permanent);
    if (permanent) {
        Mod::get()->setSavedValue(banUntilKey(accountID), 0);
    } else {
        auto now = static_cast<long long>(std::time(nullptr));
        auto until = std::min<long long>(now + durationSeconds, std::numeric_limits<int>::max());
        Mod::get()->setSavedValue(banUntilKey(accountID), static_cast<int>(until));
    }
    return true;
}

bool unbanUser(int accountID) {
    if (!isOwner() || accountID <= 0 || accountID == config::kOwnerAccountID) return false;
    bool had = Mod::get()->getSavedValue<bool>(banPermanentKey(accountID), false) ||
        Mod::get()->getSavedValue<int>(banUntilKey(accountID), 0) > 0;
    Mod::get()->setSavedValue(banPermanentKey(accountID), false);
    Mod::get()->setSavedValue(banUntilKey(accountID), 0);
    return had;
}

bool isUserPermanentlyBanned(int accountID) {
    if (accountID <= 0) return false;
    return Mod::get()->getSavedValue<bool>(banPermanentKey(accountID), false);
}

int userBanRemainingSeconds(int accountID) {
    if (accountID <= 0 || isUserPermanentlyBanned(accountID)) return 0;
    int until = Mod::get()->getSavedValue<int>(banUntilKey(accountID), 0);
    auto now = static_cast<long long>(std::time(nullptr));
    if (until <= now) return 0;
    return static_cast<int>(std::min<long long>(until - now, std::numeric_limits<int>::max()));
}

bool isUserBanned(int accountID) {
    if (accountID <= 0) return false;
    if (isUserPermanentlyBanned(accountID)) return true;
    int left = userBanRemainingSeconds(accountID);
    if (left > 0) return true;
    if (Mod::get()->getSavedValue<int>(banUntilKey(accountID), 0) > 0) {
        Mod::get()->setSavedValue(banUntilKey(accountID), 0);
    }
    return false;
}

bool setUserDisplayOverride(int accountID, std::string const& displayName) {
    if (!isOwner() || accountID <= 0 || accountID == config::kOwnerAccountID || displayName.empty()) return false;
    Mod::get()->setSavedValue(displayOverrideKey(accountID), displayName);
    return true;
}

bool clearUserDisplayOverride(int accountID) {
    if (!isOwner() || accountID <= 0 || accountID == config::kOwnerAccountID) return false;
    auto old = Mod::get()->getSavedValue<std::string>(displayOverrideKey(accountID), "");
    Mod::get()->setSavedValue(displayOverrideKey(accountID), std::string{});
    return !old.empty();
}

std::optional<std::string> userDisplayOverride(int accountID) {
    if (accountID <= 0) return std::nullopt;
    auto value = Mod::get()->getSavedValue<std::string>(displayOverrideKey(accountID), "");
    if (value.empty()) return std::nullopt;
    return value;
}

bool rateLevel(RatedLevel const& entry) {
    if (!isOwner() || entry.levelID <= 0 || !validReward(entry.reward)) return false;
    if (entry.daily && !validDailyBonus(entry.dailyBonus)) return false;

    auto levels = loadLevels();
    auto it = std::find_if(levels.begin(), levels.end(), [&](auto const& e) { return e.levelID == entry.levelID; });

    RatedLevel finalEntry = entry;
    bool wasSameDaily = it != levels.end() && it->daily && entry.daily;

    if (entry.daily) {
        if (wasSameDaily) {
            finalEntry.dailyGeneration = it->dailyGeneration;
        } else {
            int generation = Mod::get()->getSavedValue<int>("prototype-daily-generation", 0) + 1;
            Mod::get()->setSavedValue("prototype-daily-generation", generation);
            finalEntry.dailyGeneration = generation;
        }
        for (auto& e : levels) {
            if (e.levelID != entry.levelID) {
                e.daily = false;
                e.dailyBonus = 0;
                e.dailyGeneration = 0;
            }
        }
    } else {
        finalEntry.dailyBonus = 0;
        finalEntry.dailyGeneration = 0;
    }

    if (it == levels.end()) levels.push_back(finalEntry);
    else *it = finalEntry;

    saveLevels(levels);
    return true;
}

bool removeDaily(int levelID) {
    if (!isOwner() || levelID <= 0) return false;
    auto levels = loadLevels();
    auto it = std::find_if(levels.begin(), levels.end(), [&](auto const& e) { return e.levelID == levelID; });
    if (it == levels.end()) return false;
    it->daily = false;
    it->dailyBonus = 0;
    it->dailyGeneration = 0;
    saveLevels(levels);
    return true;
}

bool removeRate(int levelID) {
    if (!isOwner() || levelID <= 0) return false;
    auto levels = loadLevels();
    auto it = std::find_if(levels.begin(), levels.end(), [&](auto const& e) { return e.levelID == levelID; });
    if (it == levels.end()) return false;

    // An unrate revokes rewards that were actually claimed from this rate.
    // The balance is intentionally allowed to go negative if the player
    // already spent those Layer-amonds.
    int revoke = 0;
    if (claimedBaseImpl(levelID)) {
        revoke += it->reward;
        Mod::get()->setSavedValue(fmt::format("prototype-claim-base-{}", levelID), false);
    }
    if (it->daily && it->dailyGeneration > 0 && claimedDailyImpl(it->dailyGeneration)) {
        revoke += it->dailyBonus;
        Mod::get()->setSavedValue(fmt::format("prototype-claim-daily-{}", it->dailyGeneration), false);
    }
    if (revoke > 0) {
        auto e = economy();
        e.diamonds = std::max(-100000000, e.diamonds - revoke);
        setEconomy(e);
    }

    levels.erase(it);
    saveLevels(levels);
    return true;
}

std::vector<RatedLevel> ratedLevels() {
    return loadLevels();
}

std::optional<RatedLevel> dailyLevel() {
    auto levels = loadLevels();
    auto it = std::find_if(levels.begin(), levels.end(), [](auto const& e) { return e.daily; });
    if (it == levels.end()) return std::nullopt;
    return *it;
}

std::optional<RatedLevel> ratedLevel(int levelID) {
    auto levels = loadLevels();
    auto it = std::find_if(levels.begin(), levels.end(), [&](auto const& e) { return e.levelID == levelID; });
    if (it == levels.end()) return std::nullopt;
    return *it;
}

bool isBaseClaimed(int levelID) {
    return levelID > 0 && claimedBaseImpl(levelID);
}

bool isDailyClaimed(int dailyGeneration) {
    return claimedDailyImpl(dailyGeneration);
}


ClaimResult claimCompletion(int levelID) {
    ClaimResult result;
    auto rated = ratedLevel(levelID);
    if (!rated) return result;

    if (!claimedBaseImpl(levelID)) {
        result.base = rated->reward;
        result.firstBaseClaim = true;
        setClaimedBase(levelID);
    }

    if (rated->daily && rated->dailyGeneration > 0 && !claimedDailyImpl(rated->dailyGeneration)) {
        result.dailyBonus = rated->dailyBonus;
        result.firstDailyClaim = true;
        setClaimedDaily(rated->dailyGeneration);
    }

    result.total = result.base + result.dailyBonus;
    addDiamonds(result.total);
    return result;
}

} // namespace du::backend

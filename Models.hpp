#pragma once

#include "CoreLogic.hpp"
#include <string>
#include <vector>

namespace du {

struct RatedLevel {
    int levelID = 0;
    std::string levelName;
    std::string creatorName;
    Difficulty difficulty = Difficulty::NA;
    DemonType demonType = DemonType::Easy;
    FeatureTier feature = FeatureTier::None;
    int reward = 0;
    bool daily = false;
    int dailyBonus = 0;
    int dailyGeneration = 0;
};

struct EconomyState {
    int diamonds = 0;
    int nameExpansionKeys = 0;
    bool nameExpanded = false;
    bool stickerAccess = false;
    int stickerKeys = 0;
    int stickerSlots = 0;
};

struct ClaimResult {
    int base = 0;
    int dailyBonus = 0;
    int total = 0;
    bool firstBaseClaim = false;
    bool firstDailyClaim = false;
};

} // namespace du

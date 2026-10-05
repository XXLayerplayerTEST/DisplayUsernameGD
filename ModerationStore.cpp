#include "ModerationStore.hpp"
#include "CoreLogic.hpp"
#include <Geode/Geode.hpp>
#include <algorithm>

using namespace geode::prelude;

namespace du::moderation {
namespace {

std::vector<std::string> loadWords() {
    std::vector<std::string> out;
    auto raw = Mod::get()->getSavedValue<matjson::Value>("du-owner-blocked-words", matjson::Value::array());
    if (!raw.isArray()) return out;
    for (auto const& item : raw) {
        auto s = item.asString().unwrapOr("");
        s = du::sanitizeDisplayNameInput(s);
        s = du::trimToCodepoints(s, kMaxBlockedWordLength);
        if (s.empty()) continue;
        if (std::find(out.begin(), out.end(), s) == out.end()) out.push_back(s);
    }
    return out;
}

void saveWords(std::vector<std::string> const& words) {
    matjson::Value raw = matjson::Value::array();
    for (auto const& s : words) raw.push(s);
    Mod::get()->setSavedValue("du-owner-blocked-words", raw);
}

std::string normalized(std::string const& text) {
    return du::moderationKey(text);
}

} // namespace

std::vector<std::string> blockedWords() {
    return loadWords();
}

bool addBlockedWord(std::string const& word) {
    auto clean = du::sanitizeDisplayNameInput(word);
    clean = du::trimToCodepoints(clean, kMaxBlockedWordLength);
    if (clean.empty()) return false;

    auto key = normalized(clean);
    if (key.empty()) return false;

    auto words = loadWords();
    for (auto const& existing : words) {
        if (normalized(existing) == key) return false;
    }
    words.push_back(clean);
    saveWords(words);
    return true;
}

bool removeBlockedWord(std::string const& word) {
    auto key = normalized(word);
    if (key.empty()) return false;
    auto words = loadWords();
    auto oldSize = words.size();
    words.erase(std::remove_if(words.begin(), words.end(), [&](std::string const& s) {
        return normalized(s) == key;
    }), words.end());
    if (words.size() == oldSize) return false;
    saveWords(words);
    return true;
}

bool containsBlockedWord(std::string const& displayName) {
    auto nameKey = normalized(displayName);
    if (nameKey.empty()) return false;
    for (auto const& word : loadWords()) {
        auto key = normalized(word);
        if (!key.empty() && nameKey.find(key) != std::string::npos) return true;
    }
    return false;
}

} // namespace du::moderation

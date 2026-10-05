#pragma once

#include <string>
#include <vector>

namespace du::moderation {

constexpr int kMaxBlockedWordLength = 25;

std::vector<std::string> blockedWords();
bool addBlockedWord(std::string const& word);
bool removeBlockedWord(std::string const& word);
bool containsBlockedWord(std::string const& displayName);

} // namespace du::moderation

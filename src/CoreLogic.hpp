#pragma once

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <initializer_list>
#include <string>
#include <string_view>

namespace du {

enum class Visibility : int {
    All = 0,
    Friends = 1,
    Me = 2,
};

enum class Difficulty : int {
    NA = 0,
    Easy,
    Normal,
    Hard,
    Harder,
    Insane,
    Demon,
    Auto,
};

enum class DemonType : int {
    Easy = 0,
    Medium,
    Hard,
    Insane,
    Extreme,
};

enum class FeatureTier : int {
    None = 0,
    Featured,
    Epic,
    Legendary,
    Mythic,
};

constexpr int kBaseNameLimit = 15;
constexpr int kExpandedNameLimit = 25;
constexpr int kNameExpansionPrice = 1000;
constexpr int kStickerAccessPrice = 500;
constexpr int kStickerKeyPrice = 100;
constexpr int kMaxStickerSlots = 5;

inline char const* visibilityLabel(Visibility v) {
    switch (v) {
        case Visibility::All: return "ALL";
        case Visibility::Friends: return "FRIENDS";
        case Visibility::Me: return "ME";
    }
    return "ALL";
}

inline Visibility nextVisibility(Visibility v) {
    switch (v) {
        case Visibility::All: return Visibility::Friends;
        case Visibility::Friends: return Visibility::Me;
        case Visibility::Me: return Visibility::All;
    }
    return Visibility::All;
}

inline char const* difficultyLabel(Difficulty d) {
    switch (d) {
        case Difficulty::NA: return "N/A";
        case Difficulty::Easy: return "EASY";
        case Difficulty::Normal: return "NORMAL";
        case Difficulty::Hard: return "HARD";
        case Difficulty::Harder: return "HARDER";
        case Difficulty::Insane: return "INSANE";
        case Difficulty::Demon: return "DEMON";
        case Difficulty::Auto: return "AUTO";
    }
    return "N/A";
}

inline Difficulty nextDifficulty(Difficulty d) {
    switch (d) {
        case Difficulty::NA: return Difficulty::Easy;
        case Difficulty::Easy: return Difficulty::Normal;
        case Difficulty::Normal: return Difficulty::Hard;
        case Difficulty::Hard: return Difficulty::Harder;
        case Difficulty::Harder: return Difficulty::Insane;
        case Difficulty::Insane: return Difficulty::Demon;
        case Difficulty::Demon: return Difficulty::Auto;
        case Difficulty::Auto: return Difficulty::NA;
    }
    return Difficulty::NA;
}

inline char const* demonTypeLabel(DemonType d) {
    switch (d) {
        case DemonType::Easy: return "EASY DEMON";
        case DemonType::Medium: return "MEDIUM DEMON";
        case DemonType::Hard: return "HARD DEMON";
        case DemonType::Insane: return "INSANE DEMON";
        case DemonType::Extreme: return "EXTREME DEMON";
    }
    return "EASY DEMON";
}

inline DemonType nextDemonType(DemonType d) {
    switch (d) {
        case DemonType::Easy: return DemonType::Medium;
        case DemonType::Medium: return DemonType::Hard;
        case DemonType::Hard: return DemonType::Insane;
        case DemonType::Insane: return DemonType::Extreme;
        case DemonType::Extreme: return DemonType::Easy;
    }
    return DemonType::Easy;
}

inline char const* featureLabel(FeatureTier f) {
    switch (f) {
        case FeatureTier::None: return "NONE";
        case FeatureTier::Featured: return "FEATURED";
        case FeatureTier::Epic: return "EPIC";
        case FeatureTier::Legendary: return "LEGENDARY";
        case FeatureTier::Mythic: return "MYTHIC";
    }
    return "NONE";
}

inline FeatureTier nextFeature(FeatureTier f) {
    switch (f) {
        case FeatureTier::None: return FeatureTier::Featured;
        case FeatureTier::Featured: return FeatureTier::Epic;
        case FeatureTier::Epic: return FeatureTier::Legendary;
        case FeatureTier::Legendary: return FeatureTier::Mythic;
        case FeatureTier::Mythic: return FeatureTier::None;
    }
    return FeatureTier::None;
}




enum class NameModerationResult : int {
    Allowed = 0,
    LinkOrContact,
    Impersonation,
    UnsafeFormatting,
    BlockedTerm,
};

struct NameModerationCheck {
    NameModerationResult result = NameModerationResult::Allowed;
    char const* message = "OK";

    bool allowed() const { return result == NameModerationResult::Allowed; }
};

inline std::string asciiLower(std::string text) {
    for (auto& ch : text) {
        auto c = static_cast<unsigned char>(ch);
        if (c >= 'A' && c <= 'Z') ch = static_cast<char>(c - 'A' + 'a');
    }
    return text;
}

inline bool containsAny(std::string const& haystack, std::initializer_list<char const*> needles) {
    for (auto needle : needles) {
        if (needle && *needle && haystack.find(needle) != std::string::npos) return true;
    }
    return false;
}

// Display Username uses a conservative character whitelist. This keeps emoji,
// zero-width characters, decorative Unicode and other hard-to-moderate symbols out
// of the text field entirely. Display Username accepts Latin letters only.
inline bool isAllowedDisplayCodepoint(uint32_t cp) {
    if ((cp >= 'A' && cp <= 'Z') || (cp >= 'a' && cp <= 'z') || (cp >= '0' && cp <= '9')) return true;

    // Common accented Latin letters are allowed. Cyrillic is intentionally rejected.
    if (cp >= 0x00C0 && cp <= 0x024F) return true;

    // Small reviewed punctuation set.
    switch (cp) {
        case ' ': case '_': case '$': case '-': case '.': case '!': case '?': case '\'':
            return true;
        default:
            return false;
    }
}

inline bool decodeUtf8Codepoint(std::string const& text, size_t i, uint32_t& cp, size_t& bytes) {
    if (i >= text.size()) return false;
    auto c0 = static_cast<unsigned char>(text[i]);
    cp = 0;
    bytes = 0;

    if (c0 < 0x80) { cp = c0; bytes = 1; return true; }
    if ((c0 & 0xE0) == 0xC0) { cp = c0 & 0x1F; bytes = 2; }
    else if ((c0 & 0xF0) == 0xE0) { cp = c0 & 0x0F; bytes = 3; }
    else if ((c0 & 0xF8) == 0xF0) { cp = c0 & 0x07; bytes = 4; }
    else return false;

    if (i + bytes > text.size()) return false;
    for (size_t j = 1; j < bytes; ++j) {
        auto cx = static_cast<unsigned char>(text[i + j]);
        if ((cx & 0xC0) != 0x80) return false;
        cp = (cp << 6) | (cx & 0x3F);
    }

    if ((bytes == 2 && cp < 0x80) || (bytes == 3 && cp < 0x800) ||
        (bytes == 4 && cp < 0x10000) || (cp >= 0xD800 && cp <= 0xDFFF) || cp > 0x10FFFF) {
        return false;
    }
    return true;
}

// Called on every edit/paste. Unsupported symbols simply disappear instead of
// creating a warning/review request.
inline std::string sanitizeDisplayNameInput(std::string const& raw) {
    std::string out;
    out.reserve(raw.size());

    for (size_t i = 0; i < raw.size();) {
        uint32_t cp = 0;
        size_t bytes = 0;
        if (!decodeUtf8Codepoint(raw, i, cp, bytes)) {
            ++i;
            continue;
        }
        if (isAllowedDisplayCodepoint(cp)) out.append(raw, i, bytes);
        i += bytes;
    }
    return out;
}

inline void appendUtf8(std::string& out, uint32_t cp) {
    if (cp <= 0x7F) out.push_back(static_cast<char>(cp));
    else if (cp <= 0x7FF) {
        out.push_back(static_cast<char>(0xC0 | (cp >> 6)));
        out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
    }
    else if (cp <= 0xFFFF) {
        out.push_back(static_cast<char>(0xE0 | (cp >> 12)));
        out.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
        out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
    }
    else {
        out.push_back(static_cast<char>(0xF0 | (cp >> 18)));
        out.push_back(static_cast<char>(0x80 | ((cp >> 12) & 0x3F)));
        out.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
        out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
    }
}

inline uint32_t moderationLowerCodepoint(uint32_t cp) {
    if (cp >= 'A' && cp <= 'Z') return cp - 'A' + 'a';
    return cp;
}

inline std::string moderationKey(std::string const& raw) {
    // Remove separators and normalize common leetspeak/case so variants such as
    // b.i.t.c.h or SH1T cannot bypass the blocked-word list.
    auto clean = sanitizeDisplayNameInput(raw);
    std::string key;
    key.reserve(clean.size());

    for (size_t i = 0; i < clean.size();) {
        uint32_t cp = 0;
        size_t bytes = 0;
        if (!decodeUtf8Codepoint(clean, i, cp, bytes)) {
            ++i;
            continue;
        }
        i += bytes;
        cp = moderationLowerCodepoint(cp);

        if (cp >= 'a' && cp <= 'z') {
            key.push_back(static_cast<char>(cp));
            continue;
        }
        if (cp >= '0' && cp <= '9') {
            switch (cp) {
                case '0': key.push_back('o'); break;
                case '1': key.push_back('i'); break;
                case '3': key.push_back('e'); break;
                case '4': key.push_back('a'); break;
                case '5': key.push_back('s'); break;
                case '7': key.push_back('t'); break;
                default: key.push_back(static_cast<char>(cp)); break;
            }
            continue;
        }
        if (cp == '$') {
            key.push_back('s');
            continue;
        }
        if (cp >= 0x00C0 && cp <= 0x024F) {
            appendUtf8(key, cp);
        }
        // Other whitelisted punctuation/spaces are intentionally ignored.
    }
    return key;
}

// First-line client validation. Public multiplayer sync MUST repeat moderation
// on an authenticated server and only distribute server-approved names.
inline NameModerationCheck moderateDisplayName(std::string const& raw) {
    if (raw.empty()) return {}; // empty means use the real GD username

    auto lower = asciiLower(raw);

    // Enforce the same whitelist even if a caller bypasses the input widget.
    if (sanitizeDisplayNameInput(raw) != raw) {
        return {NameModerationResult::UnsafeFormatting, "Only Latin letters, numbers, spaces and _ $ - . ! ? ' are allowed"};
    }

    // Avoid clickable/contact-style names and common invite patterns.
    if (containsAny(lower, {
        "http://", "https://", "www.", "discord.gg", "discord.com/invite",
        "t.me/", "telegram.me/", "@everyone", "@here"
    })) {
        return {NameModerationResult::LinkOrContact, "Links, invites and mass mentions are not allowed"};
    }

    // Impersonation is refused, not auto-banned.
    if (containsAny(lower, {
        "geometry dash official", "robtop official", "gd moderator", "gd mod",
        "display username staff", "display username admin", "du moderator", "du admin"
    })) {
        return {NameModerationResult::Impersonation, "This username may look like staff impersonation"};
    }

    // Clearly blocked words are refused. This check itself never issues a ban.
    // A production server can extend this list without changing the client UI.
    auto key = moderationKey(raw);
    if (containsAny(key, {
        "fuck", "fucker", "fucking", "motherfucker",
        "shit", "bullshit", "bitch", "bastard",
        "stupid", "idiot", "dumbass", "asshole",
        "porn", "porno", "nsfw",
        "nazi", "hitler",
        // Hate / racial slurs. Keep these in the moderation key list so
        // separator and simple leetspeak bypasses are rejected as well.
        "nigger", "nigga", "n1gger", "n1gga", "negro",
        "kike", "chink", "spic", "wetback", "coon",
        "suka", "blyat", "pidor", "pizda", "huy", "hui", "nahui", "ebat", "eblan"
    })) {
        return {NameModerationResult::BlockedTerm, "This display name contains a blocked word"};
    }

    return {};
}

inline int utf8Length(std::string const& text) {
    int count = 0;
    for (unsigned char c : text) {
        if ((c & 0xC0) != 0x80) ++count;
    }
    return count;
}

inline std::string trimToCodepoints(std::string const& text, int maxCodepoints) {
    if (maxCodepoints <= 0) return {};

    std::string out;
    out.reserve(text.size());
    int count = 0;

    for (size_t i = 0; i < text.size();) {
        unsigned char c = static_cast<unsigned char>(text[i]);
        size_t bytes = 1;
        if ((c & 0xE0) == 0xC0) bytes = 2;
        else if ((c & 0xF0) == 0xE0) bytes = 3;
        else if ((c & 0xF8) == 0xF0) bytes = 4;

        if (count >= maxCodepoints || i + bytes > text.size()) break;

        if ((c & 0xC0) == 0x80 || c == '\n' || c == '\r' || c == '\t') {
            ++i;
            continue;
        }

        out.append(text, i, bytes);
        i += bytes;
        ++count;
    }
    return out;
}

inline std::string effectiveDisplayName(std::string const& overrideName, std::string const& realName) {
    return overrideName.empty() ? realName : overrideName;
}

inline bool canSee(Visibility visibility, bool isSelf, bool isFriend) {
    if (isSelf) return true;
    switch (visibility) {
        case Visibility::All: return true;
        case Visibility::Friends: return isFriend;
        case Visibility::Me: return false;
    }
    return false;
}

inline int nameLimit(bool expanded) {
    return expanded ? kExpandedNameLimit : kBaseNameLimit;
}

inline bool validReward(int value) {
    return value >= 0 && value <= 1000000;
}

inline bool validDailyBonus(int value) {
    return value >= 0 && value <= 1000000;
}

inline int completionReward(int baseReward, bool isDaily, int dailyBonus) {
    if (!validReward(baseReward) || !validDailyBonus(dailyBonus)) return 0;
    return baseReward + (isDaily ? dailyBonus : 0);
}

} // namespace du

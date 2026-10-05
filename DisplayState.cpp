#include "DisplayState.hpp"

using namespace geode::prelude;

namespace du {
namespace {
Visibility readVisibility(char const* key, Visibility fallback) {
    auto raw = Mod::get()->getSavedValue<int>(key, static_cast<int>(fallback));
    if (raw < static_cast<int>(Visibility::All) || raw > static_cast<int>(Visibility::Me)) {
        return fallback;
    }
    return static_cast<Visibility>(raw);
}
}

DraftState DisplayState::loadDraft() {
    DraftState d;
    d.displayName = getDisplayName();
    d.displayFirst = displayFirst();
    d.displayEnabled = displayEnabled();
    d.displayVisibility = displayVisibility();
    d.stickerVisibility = stickerVisibility();
    return d;
}

void DisplayState::commitDraft(DraftState const& draft) {
    setDisplayName(draft.displayName);
    setDisplayFirst(draft.displayFirst);
    setDisplayEnabled(draft.displayEnabled);
    setDisplayVisibility(draft.displayVisibility);
    setStickerVisibility(draft.stickerVisibility);
}

std::string DisplayState::getDisplayName() {
    auto value = Mod::get()->getSavedValue<std::string>("display-name", "");
    // Re-check names loaded from disk. Older builds could have saved a name
    // before a moderation rule existed, so never trust persisted text blindly.
    if (!value.empty() && !moderateDisplayName(value).allowed()) {
        Mod::get()->setSavedValue("display-name", std::string{});
        return {};
    }
    return value;
}

std::string DisplayState::effectiveName(std::string const& realName) {
    auto value = getDisplayName();
    if (!value.empty() && !moderateDisplayName(value).allowed()) return realName;
    return effectiveDisplayName(value, realName);
}

void DisplayState::setDisplayName(std::string const& value) {
    Mod::get()->setSavedValue("display-name", trimToCodepoints(value, maxCharacters()));
}

bool DisplayState::displayFirst() {
    return Mod::get()->getSavedValue<bool>("display-first", true);
}
void DisplayState::setDisplayFirst(bool v) { Mod::get()->setSavedValue("display-first", v); }

bool DisplayState::displayEnabled() {
    return Mod::get()->getSavedValue<bool>("display-enabled", true);
}
void DisplayState::setDisplayEnabled(bool v) { Mod::get()->setSavedValue("display-enabled", v); }

Visibility DisplayState::displayVisibility() { return readVisibility("display-visibility", Visibility::All); }
void DisplayState::setDisplayVisibility(Visibility v) { Mod::get()->setSavedValue("display-visibility", static_cast<int>(v)); }

Visibility DisplayState::stickerVisibility() { return readVisibility("sticker-visibility", Visibility::All); }
void DisplayState::setStickerVisibility(Visibility v) { Mod::get()->setSavedValue("sticker-visibility", static_cast<int>(v)); }

bool DisplayState::nameExpanded() { return Mod::get()->getSavedValue<bool>("name-expanded", false); }
void DisplayState::setNameExpanded(bool v) { Mod::get()->setSavedValue("name-expanded", v); }
int DisplayState::maxCharacters() { return nameLimit(nameExpanded()); }

int DisplayState::utf8Length(std::string const& text) { return du::utf8Length(text); }
std::string DisplayState::trimToCodepoints(std::string const& text, int maxCodepoints) { return du::trimToCodepoints(text, maxCodepoints); }

} // namespace du

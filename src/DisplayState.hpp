#pragma once

#include <Geode/Geode.hpp>
#include "CoreLogic.hpp"

namespace du {

struct DraftState {
    std::string displayName;
    bool displayFirst = true;
    bool displayEnabled = true;
    Visibility displayVisibility = Visibility::All;
    Visibility stickerVisibility = Visibility::All;
};

struct DisplayState {
    static DraftState loadDraft();
    static void commitDraft(DraftState const& draft);

    static std::string getDisplayName();
    static std::string effectiveName(std::string const& realName);
    static void setDisplayName(std::string const& value);

    static bool displayFirst();
    static void setDisplayFirst(bool value);

    static bool displayEnabled();
    static void setDisplayEnabled(bool value);

    static Visibility displayVisibility();
    static void setDisplayVisibility(Visibility value);

    static Visibility stickerVisibility();
    static void setStickerVisibility(Visibility value);

    static bool nameExpanded();
    static void setNameExpanded(bool value);
    static int maxCharacters();

    static int utf8Length(std::string const& text);
    static std::string trimToCodepoints(std::string const& text, int maxCodepoints);
};

} // namespace du

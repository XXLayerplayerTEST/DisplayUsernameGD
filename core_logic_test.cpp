#include "../src/CoreLogic.hpp"
#include <cassert>
#include <iostream>

int main() {
    using namespace du;

    assert(nameLimit(false) == 15);
    assert(nameLimit(true) == 25);
    assert(kNameExpansionPrice == 1000);
    assert(kStickerAccessPrice == 500);
    assert(kStickerKeyPrice == 100);

    assert(effectiveDisplayName("", "Real") == "Real");
    assert(effectiveDisplayName("Display", "Real") == "Display");

    assert(canSee(Visibility::All,false,false));
    assert(canSee(Visibility::Friends,false,true));
    assert(!canSee(Visibility::Friends,false,false));
    assert(!canSee(Visibility::Me,false,true));
    assert(canSee(Visibility::Me,true,false));

    assert(nextVisibility(Visibility::All) == Visibility::Friends);
    assert(nextVisibility(Visibility::Friends) == Visibility::Me);
    assert(nextVisibility(Visibility::Me) == Visibility::All);

    assert(nextDifficulty(Difficulty::NA) == Difficulty::Easy);
    assert(nextDifficulty(Difficulty::Insane) == Difficulty::Demon);
    assert(nextDifficulty(Difficulty::Auto) == Difficulty::NA);
    assert(nextDemonType(DemonType::Extreme) == DemonType::Easy);

    assert(nextFeature(FeatureTier::Mythic) == FeatureTier::None);

    assert(utf8Length("abc") == 3);
    assert(utf8Length("абв") == 3);
    assert(trimToCodepoints("abcdef",3) == "abc");
    assert(utf8Length(trimToCodepoints("абвгд",3)) == 3);

    assert(moderateDisplayName("LayerPlayer").allowed());
    assert(moderateDisplayName("").allowed());
    assert(!moderateDisplayName("https://example.com").allowed());
    assert(!moderateDisplayName("DU Moderator").allowed());
    assert(!moderateDisplayName("<c>name</c>").allowed());
    assert(moderateDisplayName("Layer_$-!? 123").allowed());
    assert(!moderateDisplayName("Привет_123").allowed());
    assert(sanitizeDisplayNameInput("Привет_123") == "_123");
    assert(sanitizeDisplayNameInput("Layer😀_$") == "Layer_$");
    assert(sanitizeDisplayNameInput("Name★Test") == "NameTest");
    assert(!moderateDisplayName("b.i.t.c.h").allowed());
    assert(!moderateDisplayName("sh1t").allowed());
    assert(!moderateDisplayName("n@zi").allowed());
    assert(!moderateDisplayName("a$$hole").allowed());
    assert(!moderateDisplayName("nigger").allowed());
    assert(!moderateDisplayName("n.i.g.g.e.r").allowed());
    assert(!moderateDisplayName("n1gger").allowed());

    assert(validReward(0));
    assert(validReward(1000));
    assert(!validReward(-1));
    assert(completionReward(1000,true,250) == 1250);
    assert(completionReward(1000,false,250) == 1000);

    std::cout << "core logic tests passed\n";
    return 0;
}

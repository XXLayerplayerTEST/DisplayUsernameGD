#include "LevelsPopup.hpp"
#include "PrototypeBackend.hpp"

using namespace geode::prelude;

namespace {
void openOnlineSearch(std::string const& query, int pendingLevel = 0) {
    if (query.empty()) return;
    Mod::get()->setSavedValue("du-pending-level-open", pendingLevel);
    auto search = GJSearchObject::create(SearchType::Search, query);
    if (!search) return;
    auto scene = LevelBrowserLayer::scene(search);
    if (scene) CCDirector::sharedDirector()->replaceScene(scene);
}

void openRatedLevel(int levelID) {
    if (levelID <= 0) return;
    openOnlineSearch(fmt::format("{}", levelID), levelID);
}

std::string buildRatedIDsQuery() {
    auto levels = du::backend::ratedLevels();
    std::string query;
    for (auto const& e : levels) {
        if (e.levelID <= 0) continue;
        if (!query.empty()) query += ",";
        query += fmt::format("{}", e.levelID);
    }
    return query;
}
}

DULevelsPopup* DULevelsPopup::create() {
    auto ret = new DULevelsPopup();
    if (ret && ret->init()) { ret->autorelease(); return ret; }
    CC_SAFE_DELETE(ret);
    return nullptr;
}

bool DULevelsPopup::init() {
    if (!Popup::init(350.f, 220.f)) return false;
    setTitle("LAYER-AMOND LEVELS");

    auto diamond = CCSprite::create("purple-diamond.png"_spr);
    if (diamond) { diamond->setScale(.16f); diamond->setPosition({124.f,172.f}); m_mainLayer->addChild(diamond); }
    m_balance = CCLabelBMFont::create("0", "bigFont.fnt");
    m_balance->setScale(.55f); m_balance->setAnchorPoint({0.f,.5f}); m_balance->setPosition({146.f,172.f}); m_mainLayer->addChild(m_balance);

    auto dailySpr = ButtonSprite::create("DAILY"); dailySpr->setScale(.90f);
    auto dailyBtn = CCMenuItemSpriteExtra::create(dailySpr, this, menu_selector(DULevelsPopup::onDaily));
    dailyBtn->setPosition({105.f,93.f}); m_buttonMenu->addChild(dailyBtn);

    auto searchSpr = ButtonSprite::create("SEARCH"); searchSpr->setScale(.90f);
    auto searchBtn = CCMenuItemSpriteExtra::create(searchSpr, this, menu_selector(DULevelsPopup::onSearch));
    searchBtn->setPosition({245.f,93.f}); m_buttonMenu->addChild(searchBtn);

    auto hint = CCLabelBMFont::create("SEARCH opens the normal GD level list", "chatFont.fnt");
    hint->setScale(.38f); hint->setPosition({175.f,48.f}); m_mainLayer->addChild(hint);

    refreshBalance();
    return true;
}

void DULevelsPopup::refreshBalance() {
    if (m_balance) m_balance->setString(fmt::format("{}", du::backend::economy().diamonds).c_str());
}

void DULevelsPopup::onDaily(CCObject*) {
    auto e = du::backend::dailyLevel();
    if (!e) {
        FLAlertLayer::create("Daily", "No Daily level has been selected yet.", "OK")->show();
        return;
    }
    onClose(nullptr);
    openRatedLevel(e->levelID);
}

void DULevelsPopup::onSearch(CCObject*) {
    auto query = buildRatedIDsQuery();
    if (query.empty()) {
        FLAlertLayer::create("Search", "No Layer-amond rated levels yet.", "OK")->show();
        return;
    }
    onClose(nullptr);
    openOnlineSearch(query);
}

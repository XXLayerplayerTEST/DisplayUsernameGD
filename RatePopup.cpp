#include "RatePopup.hpp"
#include "PrototypeBackend.hpp"
#include <Geode/ui/Notification.hpp>

using namespace geode::prelude;

namespace {
int parseInt(std::string const& s) {
    if (s.empty()) return 0;
    try { return std::stoi(s); } catch (...) { return -1; }
}
}

DURatePopup* DURatePopup::create(GJGameLevel* level) {
    auto ret = new DURatePopup();
    if (ret && ret->init(level)) { ret->autorelease(); return ret; }
    CC_SAFE_DELETE(ret);
    return nullptr;
}

bool DURatePopup::init(GJGameLevel* level) {
    if (!level || !Popup::init(390.f, 315.f)) return false;
    m_level = level;
    setTitle("DIAMOND RATE");

    if (auto existing = du::backend::ratedLevel(level->m_levelID.value())) {
        m_difficulty = existing->difficulty;
        m_demonType = existing->demonType;
        m_feature = existing->feature;
        m_daily = existing->daily;
    }

    auto id = CCLabelBMFont::create(fmt::format("LEVEL {}", level->m_levelID.value()).c_str(), "chatFont.fnt");
    id->setScale(.50f); id->setPosition({195.f,267.f}); m_mainLayer->addChild(id);

    auto mkLabel = [&](char const* text, float x, float y) {
        auto l = CCLabelBMFont::create(text, "goldFont.fnt");
        l->setScale(.34f); l->setAnchorPoint({0.f,.5f}); l->setPosition({x,y}); m_mainLayer->addChild(l); return l;
    };

    mkLabel("DU DIFFICULTY", 28.f, 225.f);
    auto diffSpr = ButtonSprite::create("N/A"); diffSpr->setScale(.56f);
    m_diffButton = CCMenuItemSpriteExtra::create(diffSpr, this, menu_selector(DURatePopup::onDifficulty));
    m_diffButton->setPosition({286.f,225.f}); m_buttonMenu->addChild(m_diffButton);

    auto demonSpr = ButtonSprite::create("EASY DEMON"); demonSpr->setScale(.50f);
    m_demonButton = CCMenuItemSpriteExtra::create(demonSpr, this, menu_selector(DURatePopup::onDemonType));
    m_demonButton->setPosition({286.f,195.f}); m_buttonMenu->addChild(m_demonButton);

    mkLabel("DU FEATURE", 28.f, 164.f);
    auto featureSpr = ButtonSprite::create("NONE"); featureSpr->setScale(.56f);
    m_featureButton = CCMenuItemSpriteExtra::create(featureSpr, this, menu_selector(DURatePopup::onFeature));
    m_featureButton->setPosition({286.f,164.f}); m_buttonMenu->addChild(m_featureButton);

    mkLabel("REWARD", 28.f, 129.f);
    m_rewardInput = TextInput::create(115.f, "Layer-amonds", "bigFont.fnt");
    m_rewardInput->setCommonFilter(CommonFilter::Uint); m_rewardInput->setMaxCharCount(7);
    m_rewardInput->setPosition({286.f,129.f}); m_mainLayer->addChild(m_rewardInput);

    auto dailySpr = ButtonSprite::create("DAILY: OFF"); dailySpr->setScale(.56f);
    m_dailyButton = CCMenuItemSpriteExtra::create(dailySpr, this, menu_selector(DURatePopup::onDaily));
    m_dailyButton->setPosition({95.f,92.f}); m_buttonMenu->addChild(m_dailyButton);

    mkLabel("BONUS", 175.f, 92.f);
    m_bonusInput = TextInput::create(92.f, "Bonus", "bigFont.fnt");
    m_bonusInput->setCommonFilter(CommonFilter::Uint); m_bonusInput->setMaxCharCount(7);
    m_bonusInput->setPosition({296.f,92.f}); m_mainLayer->addChild(m_bonusInput);

    auto rateSpr = ButtonSprite::create("RATE IT!"); rateSpr->setScale(.64f);
    auto rateBtn = CCMenuItemSpriteExtra::create(rateSpr, this, menu_selector(DURatePopup::onRate));
    rateBtn->setPosition({195.f,54.f}); m_buttonMenu->addChild(rateBtn);

    auto removeDailySpr = ButtonSprite::create("REMOVE DAILY"); removeDailySpr->setScale(.43f);
    m_removeDailyButton = CCMenuItemSpriteExtra::create(removeDailySpr, this, menu_selector(DURatePopup::onRemoveDaily));
    m_removeDailyButton->setPosition({105.f,22.f}); m_buttonMenu->addChild(m_removeDailyButton);

    auto removeRateSpr = ButtonSprite::create("REMOVE RATE"); removeRateSpr->setScale(.43f);
    m_removeRateButton = CCMenuItemSpriteExtra::create(removeRateSpr, this, menu_selector(DURatePopup::onRemoveRate));
    m_removeRateButton->setPosition({285.f,22.f}); m_buttonMenu->addChild(m_removeRateButton);

    if (auto existing = du::backend::ratedLevel(level->m_levelID.value())) {
        m_rewardInput->setString(fmt::format("{}", existing->reward), false);
        if (existing->daily) m_bonusInput->setString(fmt::format("{}", existing->dailyBonus), false);
    }

    refresh();
    return true;
}

void DURatePopup::refresh() {
    if (m_diffButton) if (auto spr = typeinfo_cast<ButtonSprite*>(m_diffButton->getNormalImage())) spr->setString(du::difficultyLabel(m_difficulty));
    bool demon = m_difficulty == du::Difficulty::Demon;
    if (m_demonButton) {
        m_demonButton->setVisible(demon);
        if (auto spr = typeinfo_cast<ButtonSprite*>(m_demonButton->getNormalImage())) spr->setString(du::demonTypeLabel(m_demonType));
    }
    if (m_featureButton) if (auto spr = typeinfo_cast<ButtonSprite*>(m_featureButton->getNormalImage())) spr->setString(du::featureLabel(m_feature));
    if (m_dailyButton) if (auto spr = typeinfo_cast<ButtonSprite*>(m_dailyButton->getNormalImage())) spr->setString(m_daily ? "DAILY: ON" : "DAILY: OFF");
    if (m_bonusInput) m_bonusInput->setVisible(m_daily);

    auto existing = m_level ? du::backend::ratedLevel(m_level->m_levelID.value()) : std::nullopt;
    if (m_removeDailyButton) {
        bool enabled = existing && existing->daily;
        m_removeDailyButton->setEnabled(enabled);
        m_removeDailyButton->setOpacity(enabled ? 255 : 110);
    }
    if (m_removeRateButton) {
        bool enabled = existing.has_value();
        m_removeRateButton->setEnabled(enabled);
        m_removeRateButton->setOpacity(enabled ? 255 : 110);
    }
}

void DURatePopup::onDifficulty(CCObject*) { m_difficulty = du::nextDifficulty(m_difficulty); refresh(); }
void DURatePopup::onDemonType(CCObject*) { m_demonType = du::nextDemonType(m_demonType); refresh(); }
void DURatePopup::onFeature(CCObject*) { m_feature = du::nextFeature(m_feature); refresh(); }
void DURatePopup::onDaily(CCObject*) { m_daily = !m_daily; refresh(); }

void DURatePopup::onRate(CCObject*) {
    auto reward = parseInt(m_rewardInput ? m_rewardInput->getString() : "");
    auto bonus = m_daily ? parseInt(m_bonusInput ? m_bonusInput->getString() : "") : 0;
    if (!du::validReward(reward) || !du::validDailyBonus(bonus)) {
        Notification::create("Invalid reward or bonus", NotificationIcon::Error)->show();
        return;
    }

    du::RatedLevel e;
    e.levelID = m_level->m_levelID.value();
    e.levelName = std::string(m_level->m_levelName);
    e.creatorName = std::string(m_level->m_creatorName);
    e.difficulty = m_difficulty;
    e.demonType = m_demonType;
    e.feature = m_feature;
    e.reward = reward;
    e.daily = m_daily;
    e.dailyBonus = bonus;

    if (!du::backend::rateLevel(e)) {
        Notification::create("Rate failed: owner only", NotificationIcon::Error)->show();
        return;
    }
    Notification::create(m_daily ? "Rated + set as Daily" : "Diamond rate saved", NotificationIcon::Success)->show();
    onClose(nullptr);
}

void DURatePopup::onRemoveDaily(CCObject*) {
    if (!m_level || !du::backend::removeDaily(m_level->m_levelID.value())) {
        Notification::create("This level is not Daily", NotificationIcon::Error)->show();
        return;
    }
    m_daily = false;
    Notification::create("Daily removed; diamond rate kept", NotificationIcon::Success)->show();
    refresh();
}

void DURatePopup::onRemoveRate(CCObject*) {
    if (!m_level || !du::backend::removeRate(m_level->m_levelID.value())) {
        Notification::create("No diamond rate to remove", NotificationIcon::Error)->show();
        return;
    }
    Notification::create("Diamond rate removed; claimed rewards revoked", NotificationIcon::Success)->show();
    onClose(nullptr);
}

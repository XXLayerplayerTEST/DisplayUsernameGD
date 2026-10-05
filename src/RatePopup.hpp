#pragma once
#include <Geode/Geode.hpp>
#include <Geode/ui/Popup.hpp>
#include <Geode/ui/TextInput.hpp>
#include "Models.hpp"

class DURatePopup : public geode::Popup {
protected:
    bool init(GJGameLevel* level);
    void refresh();
    void onDifficulty(cocos2d::CCObject*);
    void onDemonType(cocos2d::CCObject*);
    void onFeature(cocos2d::CCObject*);
    void onDaily(cocos2d::CCObject*);
    void onRate(cocos2d::CCObject*);
    void onRemoveDaily(cocos2d::CCObject*);
    void onRemoveRate(cocos2d::CCObject*);

    GJGameLevel* m_level = nullptr;
    du::Difficulty m_difficulty = du::Difficulty::NA;
    du::DemonType m_demonType = du::DemonType::Easy;
    du::FeatureTier m_feature = du::FeatureTier::None;
    bool m_daily = false;
    geode::TextInput* m_rewardInput = nullptr;
    geode::TextInput* m_bonusInput = nullptr;
    CCMenuItemSpriteExtra* m_diffButton = nullptr;
    CCMenuItemSpriteExtra* m_demonButton = nullptr;
    CCMenuItemSpriteExtra* m_featureButton = nullptr;
    CCMenuItemSpriteExtra* m_dailyButton = nullptr;
    CCMenuItemSpriteExtra* m_removeDailyButton = nullptr;
    CCMenuItemSpriteExtra* m_removeRateButton = nullptr;
public:
    static DURatePopup* create(GJGameLevel* level);
};

#pragma once

#include <Geode/Geode.hpp>
#include <Geode/ui/Popup.hpp>
#include <Geode/ui/TextInput.hpp>
#include "DisplayState.hpp"

class DUDisplaySettingsPopup : public geode::Popup {
protected:
    bool init();

    void updateCounter();
    void refresh();
    void updatePrivacyButton(CCMenuItemSpriteExtra* button, du::Visibility visibility);

    void onShop(cocos2d::CCObject*);
    void onStickers(cocos2d::CCObject*);
    void onMore(cocos2d::CCObject*);
    void onDisplayPrivacy(cocos2d::CCObject*);
    void onStickerPrivacy(cocos2d::CCObject*);
    void onShowMode(cocos2d::CCObject*);
    void onDisableDisplay(cocos2d::CCObject*);
    void onHelp(cocos2d::CCObject*);
    void onModeration(cocos2d::CCObject*);
    void onSave(cocos2d::CCObject*);

    geode::TextInput* m_displayInput = nullptr;
    cocos2d::CCLabelBMFont* m_counter = nullptr;
    cocos2d::CCLabelBMFont* m_balance = nullptr;
    CCMenuItemSpriteExtra* m_stickerButton = nullptr;
    CCMenuItemSpriteExtra* m_displayPrivacyButton = nullptr;
    CCMenuItemSpriteExtra* m_stickerPrivacyButton = nullptr;
    CCMenuItemSpriteExtra* m_showModeButton = nullptr;
    CCMenuItemSpriteExtra* m_disableDisplayButton = nullptr;

    du::DraftState m_draft;
    std::string m_realName;

public:
    static DUDisplaySettingsPopup* create();
};

#pragma once

#include <Geode/Geode.hpp>
#include <Geode/ui/Popup.hpp>
#include <Geode/ui/TextInput.hpp>
#include <string>

class DUBanUserPopup : public geode::Popup {
protected:
    bool init(int accountID, std::string username);
    void refresh();
    void onUnit(cocos2d::CCObject*);
    void onBan(cocos2d::CCObject*);
    void onUnban(cocos2d::CCObject*);

    int m_accountID = 0;
    std::string m_username;
    int m_unit = 0;
    geode::TextInput* m_amountInput = nullptr;
    CCMenuItemSpriteExtra* m_unitButton = nullptr;
    cocos2d::CCLabelBMFont* m_statusLabel = nullptr;

public:
    static DUBanUserPopup* create(int accountID, std::string username);
};

class DUChangeDisplayPopup : public geode::Popup {
protected:
    bool init(int accountID, std::string username);
    void onSave(cocos2d::CCObject*);
    void onUseReal(cocos2d::CCObject*);

    int m_accountID = 0;
    std::string m_username;
    geode::TextInput* m_input = nullptr;

public:
    static DUChangeDisplayPopup* create(int accountID, std::string username);
};

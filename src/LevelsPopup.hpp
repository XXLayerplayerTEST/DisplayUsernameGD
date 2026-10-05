#pragma once
#include <Geode/Geode.hpp>
#include <Geode/ui/Popup.hpp>

class DULevelsPopup : public geode::Popup {
protected:
    bool init();
    void refreshBalance();
    void onDaily(cocos2d::CCObject*);
    void onSearch(cocos2d::CCObject*);
    cocos2d::CCLabelBMFont* m_balance = nullptr;
public:
    static DULevelsPopup* create();
};

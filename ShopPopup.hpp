#pragma once
#include <Geode/Geode.hpp>
#include <Geode/ui/Popup.hpp>
#include <functional>

class DUShopPopup : public geode::Popup {
protected:
    bool init(std::function<void()> onChanged);
    void refresh();
    void onBuyNameKey(cocos2d::CCObject*);
    void onStickerAccess(cocos2d::CCObject*);
    void onStickerKey(cocos2d::CCObject*);

    cocos2d::CCLabelBMFont* m_balance = nullptr;
    cocos2d::CCLabelBMFont* m_nameStatus = nullptr;
    cocos2d::CCLabelBMFont* m_accessStatus = nullptr;
    cocos2d::CCLabelBMFont* m_stickerKeyStatus = nullptr;
    CCMenuItemSpriteExtra* m_nameButton = nullptr;
    CCMenuItemSpriteExtra* m_accessButton = nullptr;
    CCMenuItemSpriteExtra* m_stickerKeyButton = nullptr;
    std::function<void()> m_onChanged;
public:
    static DUShopPopup* create(std::function<void()> onChanged = {});
};

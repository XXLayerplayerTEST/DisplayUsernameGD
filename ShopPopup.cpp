#include "ShopPopup.hpp"
#include "CoreLogic.hpp"
#include "PrototypeBackend.hpp"
#include "Config.hpp"
#include <Geode/ui/Notification.hpp>

using namespace geode::prelude;

DUShopPopup* DUShopPopup::create(std::function<void()> onChanged) {
    auto ret = new DUShopPopup();
    if (ret && ret->init(std::move(onChanged))) { ret->autorelease(); return ret; }
    CC_SAFE_DELETE(ret);
    return nullptr;
}

bool DUShopPopup::init(std::function<void()> onChanged) {
    if (!Popup::init(370.f, 250.f)) return false;
    m_onChanged = std::move(onChanged);
    setTitle("DISPLAY SHOP");

    auto diamond = CCSprite::create("purple-diamond.png"_spr);
    if (diamond) {
        diamond->setScale(.16f);
        diamond->setPosition({113.f, 208.f});
        m_mainLayer->addChild(diamond);
    }
    m_balance = CCLabelBMFont::create("0", "bigFont.fnt");
    m_balance->setScale(.55f);
    m_balance->setAnchorPoint({0.f, .5f});
    m_balance->setPosition({130.f, 208.f});
    m_mainLayer->addChild(m_balance);

    auto addRow = [&](float y, char const* title, CCLabelBMFont*& status, CCMenuItemSpriteExtra*& button, SEL_MenuHandler handler) {
        auto t = CCLabelBMFont::create(title, "goldFont.fnt");
        t->setScale(.38f); t->setAnchorPoint({0.f,.5f}); t->setPosition({35.f,y+13.f}); m_mainLayer->addChild(t);
        status = CCLabelBMFont::create("", "chatFont.fnt");
        status->setScale(.44f); status->setAnchorPoint({0.f,.5f}); status->setPosition({35.f,y-10.f}); m_mainLayer->addChild(status);
        auto spr = ButtonSprite::create("BUY"); spr->setScale(.62f);
        button = CCMenuItemSpriteExtra::create(spr, this, handler); button->setPosition({315.f,y}); m_buttonMenu->addChild(button);
    };

    addRow(158.f, "NAME EXPANSION KEY - 1000", m_nameStatus, m_nameButton, menu_selector(DUShopPopup::onBuyNameKey));
    addRow(105.f, "STICKER ACCESS KEY - 500", m_accessStatus, m_accessButton, menu_selector(DUShopPopup::onStickerAccess));
    addRow(52.f, "STICKER KEY - 100", m_stickerKeyStatus, m_stickerKeyButton, menu_selector(DUShopPopup::onStickerKey));

    // Stickers are visibly locked until 2.209 support exists.
    auto addLock = [&](CCMenuItemSpriteExtra* button) {
        if (!button) return;
        if (auto sprite = typeinfo_cast<ButtonSprite*>(button->getNormalImage())) {
            sprite->setColor(ccc3(105,105,105));
            sprite->setOpacity(190);
            if (auto lock = CCSprite::createWithSpriteFrameName("GJ_lockGray_001.png")) {
                lock->setScale(.50f);
                auto cs = sprite->getContentSize();
                lock->setPosition({cs.width * .5f, cs.height * .5f});
                sprite->addChild(lock, 50);
            }
        }
    };
    addLock(m_accessButton);
    addLock(m_stickerKeyButton);

    refresh();
    return true;
}

void DUShopPopup::refresh() {
    auto e = du::backend::economy();
    if (m_balance) m_balance->setString(fmt::format("{}", e.diamonds).c_str());

    if (e.nameExpanded) {
        m_nameStatus->setString("UNLOCKED - LIMIT 25");
        m_nameButton->setEnabled(false); m_nameButton->setOpacity(120);
    } else {
        m_nameStatus->setString("BUY = LIMIT 15 -> 25 INSTANTLY");
        bool can = e.diamonds >= du::kNameExpansionPrice;
        m_nameButton->setEnabled(can); m_nameButton->setOpacity(can ? 255 : 120);
    }

    auto stickerText = du::config::kStickersAvailable ? "READY" : "COMING SOON - GD 2.209";
    m_accessStatus->setString(stickerText);
    m_stickerKeyStatus->setString(stickerText);
    if (!du::config::kStickersAvailable) {
        m_accessStatus->setColor(ccc3(150,150,150));
        m_stickerKeyStatus->setColor(ccc3(150,150,150));
    }
    m_accessButton->setEnabled(du::config::kStickersAvailable);
    m_stickerKeyButton->setEnabled(du::config::kStickersAvailable && e.stickerAccess);
    m_accessButton->setOpacity(m_accessButton->isEnabled() ? 255 : 120);
    m_stickerKeyButton->setOpacity(m_stickerKeyButton->isEnabled() ? 255 : 120);
}

void DUShopPopup::onBuyNameKey(CCObject*) {
    if (!du::backend::buyNameExpansionKey()) {
        Notification::create("Not enough Layer-amonds or key already owned", NotificationIcon::Error)->show();
        return;
    }
    Notification::create("Display Username limit increased to 25", NotificationIcon::Success)->show();
    if (m_onChanged) m_onChanged();
    refresh();
}

void DUShopPopup::onStickerAccess(CCObject*) {
    if (!du::config::kStickersAvailable) {
        FLAlertLayer::create("Stickers", "<cy>COMING SOON</c> until the official 2.209 sticker system is available.", "OK")->show();
        return;
    }
    if (du::backend::buyStickerAccessKey()) Notification::create("Sticker access unlocked", NotificationIcon::Success)->show();
    refresh();
}

void DUShopPopup::onStickerKey(CCObject*) {
    if (!du::config::kStickersAvailable) {
        FLAlertLayer::create("Stickers", "<cy>COMING SOON</c> until the official 2.209 sticker system is available.", "OK")->show();
        return;
    }
    if (du::backend::buyStickerKey()) Notification::create("Sticker Key purchased", NotificationIcon::Success)->show();
    refresh();
}

#include "ProfileModerationPopup.hpp"
#include "PrototypeBackend.hpp"
#include "CoreLogic.hpp"
#include "ModerationStore.hpp"
#include <Geode/ui/Notification.hpp>
#include <algorithm>
#include <limits>

using namespace geode::prelude;

namespace {
constexpr int kUnitSeconds = 0;
constexpr int kUnitMinutes = 1;
constexpr int kUnitHours = 2;
constexpr int kUnitDays = 3;
constexpr int kUnitMonths = 4;
constexpr int kUnitPermanent = 5;

char const* unitLabel(int unit) {
    switch (unit) {
        case kUnitSeconds: return "SECONDS";
        case kUnitMinutes: return "MINUTES";
        case kUnitHours: return "HOURS";
        case kUnitDays: return "DAYS";
        case kUnitMonths: return "MONTHS";
        case kUnitPermanent: return "PERMANENT";
        default: return "SECONDS";
    }
}

long long secondsPerUnit(int unit) {
    switch (unit) {
        case kUnitSeconds: return 1LL;
        case kUnitMinutes: return 60LL;
        case kUnitHours: return 60LL * 60LL;
        case kUnitDays: return 24LL * 60LL * 60LL;
        case kUnitMonths: return 30LL * 24LL * 60LL * 60LL;
        default: return 1LL;
    }
}

int parsePositiveInt(std::string const& text) {
    if (text.empty()) return 0;
    try {
        auto value = std::stoll(text);
        if (value <= 0) return 0;
        return static_cast<int>(std::min<long long>(value, 1000000LL));
    } catch (...) {
        return 0;
    }
}
}

DUBanUserPopup* DUBanUserPopup::create(int accountID, std::string username) {
    auto ret = new DUBanUserPopup();
    if (ret && ret->init(accountID, std::move(username))) {
        ret->autorelease();
        return ret;
    }
    CC_SAFE_DELETE(ret);
    return nullptr;
}

bool DUBanUserPopup::init(int accountID, std::string username) {
    if (!du::backend::isOwner() || accountID <= 0 || !Popup::init(330.f, 235.f)) return false;
    m_accountID = accountID;
    m_username = std::move(username);
    setTitle("MOD BAN");

    auto who = CCLabelBMFont::create(fmt::format("{}  #{}", m_username, m_accountID).c_str(), "chatFont.fnt");
    who->setScale(.48f);
    who->setPosition({165.f, 190.f});
    m_mainLayer->addChild(who);

    m_statusLabel = CCLabelBMFont::create("NOT BANNED", "goldFont.fnt");
    m_statusLabel->setScale(.38f);
    m_statusLabel->setPosition({165.f, 164.f});
    m_mainLayer->addChild(m_statusLabel);

    m_amountInput = TextInput::create(92.f, "AMOUNT", "bigFont.fnt");
    m_amountInput->setCommonFilter(CommonFilter::Uint);
    m_amountInput->setMaxCharCount(6);
    m_amountInput->setPosition({94.f, 126.f});
    m_mainLayer->addChild(m_amountInput);

    auto unitSpr = ButtonSprite::create("SECONDS");
    unitSpr->setScale(.50f);
    m_unitButton = CCMenuItemSpriteExtra::create(unitSpr, this, menu_selector(DUBanUserPopup::onUnit));
    m_unitButton->setPosition({230.f, 126.f});
    m_buttonMenu->addChild(m_unitButton);

    auto banSpr = ButtonSprite::create("BAN");
    banSpr->setScale(.60f);
    auto banBtn = CCMenuItemSpriteExtra::create(banSpr, this, menu_selector(DUBanUserPopup::onBan));
    banBtn->setPosition({102.f, 72.f});
    m_buttonMenu->addChild(banBtn);

    auto unbanSpr = ButtonSprite::create("UNBAN");
    unbanSpr->setScale(.52f);
    auto unbanBtn = CCMenuItemSpriteExtra::create(unbanSpr, this, menu_selector(DUBanUserPopup::onUnban));
    unbanBtn->setPosition({228.f, 72.f});
    m_buttonMenu->addChild(unbanBtn);

    auto hint = CCLabelBMFont::create("Owner-only prototype; server enforcement required for release", "chatFont.fnt");
    hint->setScale(.26f);
    hint->setOpacity(150);
    hint->setPosition({165.f, 35.f});
    m_mainLayer->addChild(hint);

    refresh();
    return true;
}

void DUBanUserPopup::refresh() {
    if (m_unitButton) {
        if (auto spr = typeinfo_cast<ButtonSprite*>(m_unitButton->getNormalImage())) spr->setString(unitLabel(m_unit));
    }
    if (m_amountInput) m_amountInput->setVisible(m_unit != kUnitPermanent);

    if (m_statusLabel) {
        if (du::backend::isUserBanned(m_accountID)) {
            if (du::backend::isUserPermanentlyBanned(m_accountID)) {
                m_statusLabel->setString("BANNED: PERMANENT");
            } else {
                int left = du::backend::userBanRemainingSeconds(m_accountID);
                m_statusLabel->setString(fmt::format("BANNED: {}s LEFT", left).c_str());
            }
        } else {
            m_statusLabel->setString("NOT BANNED");
        }
    }
}

void DUBanUserPopup::onUnit(CCObject*) {
    m_unit = (m_unit + 1) % 6;
    refresh();
}

void DUBanUserPopup::onBan(CCObject*) {
    if (m_unit == kUnitPermanent) {
        if (!du::backend::banUser(m_accountID, 0, true)) {
            Notification::create("Ban failed", NotificationIcon::Error)->show();
            return;
        }
    } else {
        int amount = parsePositiveInt(m_amountInput ? m_amountInput->getString() : "");
        if (amount <= 0) {
            Notification::create("Enter a positive amount", NotificationIcon::Warning)->show();
            return;
        }
        long long total = static_cast<long long>(amount) * secondsPerUnit(m_unit);
        total = std::min<long long>(total, std::numeric_limits<int>::max() / 2);
        if (!du::backend::banUser(m_accountID, static_cast<int>(total), false)) {
            Notification::create("Ban failed", NotificationIcon::Error)->show();
            return;
        }
    }
    Notification::create("User moderation ban saved", NotificationIcon::Success)->show();
    refresh();
}

void DUBanUserPopup::onUnban(CCObject*) {
    if (!du::backend::unbanUser(m_accountID)) {
        Notification::create("User was not banned", NotificationIcon::Warning)->show();
        return;
    }
    Notification::create("User unbanned", NotificationIcon::Success)->show();
    refresh();
}

DUChangeDisplayPopup* DUChangeDisplayPopup::create(int accountID, std::string username) {
    auto ret = new DUChangeDisplayPopup();
    if (ret && ret->init(accountID, std::move(username))) {
        ret->autorelease();
        return ret;
    }
    CC_SAFE_DELETE(ret);
    return nullptr;
}

bool DUChangeDisplayPopup::init(int accountID, std::string username) {
    if (!du::backend::isOwner() || accountID <= 0 || !Popup::init(330.f, 205.f)) return false;
    m_accountID = accountID;
    m_username = std::move(username);
    setTitle("CHANGE DISPLAY");

    auto who = CCLabelBMFont::create(fmt::format("{}  #{}", m_username, m_accountID).c_str(), "chatFont.fnt");
    who->setScale(.45f);
    who->setPosition({165.f, 157.f});
    m_mainLayer->addChild(who);

    m_input = TextInput::create(235.f, "DISPLAY USERNAME", "bigFont.fnt");
    m_input->setMaxCharCount(25);
    m_input->setPosition({165.f, 119.f});
    m_input->setCallback([this](std::string const& raw) {
        auto clean = du::sanitizeDisplayNameInput(raw);
        clean = du::trimToCodepoints(clean, 25);
        if (clean != raw && m_input) m_input->setString(clean, false);
    });
    if (auto existing = du::backend::userDisplayOverride(m_accountID)) m_input->setString(*existing, false);
    m_mainLayer->addChild(m_input);

    auto saveSpr = ButtonSprite::create("SAVE");
    saveSpr->setScale(.58f);
    auto save = CCMenuItemSpriteExtra::create(saveSpr, this, menu_selector(DUChangeDisplayPopup::onSave));
    save->setPosition({103.f, 63.f});
    m_buttonMenu->addChild(save);

    auto realSpr = ButtonSprite::create("USE REAL");
    realSpr->setScale(.50f);
    auto real = CCMenuItemSpriteExtra::create(realSpr, this, menu_selector(DUChangeDisplayPopup::onUseReal));
    real->setPosition({230.f, 63.f});
    m_buttonMenu->addChild(real);

    return true;
}

void DUChangeDisplayPopup::onSave(CCObject*) {
   std::string text = m_input ? std::string(m_input->getString()) : std::string{}; ? m_input->getString() : std::string{};
    text = du::sanitizeDisplayNameInput(text);
    text = du::trimToCodepoints(text, 25);
    if (text.empty()) {
        Notification::create("Enter a display name or use REAL", NotificationIcon::Warning)->show();
        return;
    }
    auto check = du::moderateDisplayName(text);
    if (!check.allowed() || du::moderation::containsBlockedWord(text)) {
        Notification::create("That display name is blocked", NotificationIcon::Error)->show();
        return;
    }
    if (!du::backend::setUserDisplayOverride(m_accountID, text)) {
        Notification::create("Change failed", NotificationIcon::Error)->show();
        return;
    }
    Notification::create("Display override saved", NotificationIcon::Success)->show();
    onClose(nullptr);
}

void DUChangeDisplayPopup::onUseReal(CCObject*) {
    if (!du::backend::clearUserDisplayOverride(m_accountID)) {
        Notification::create("Nothing to clear", NotificationIcon::Warning)->show();
        return;
    }
    Notification::create("Real GD username restored", NotificationIcon::Success)->show();
    onClose(nullptr);
}

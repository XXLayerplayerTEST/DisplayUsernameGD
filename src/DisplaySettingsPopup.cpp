#include "DisplaySettingsPopup.hpp"

#include "ShopPopup.hpp"
#include "LevelsPopup.hpp"
#include "Sync.hpp"
#include "Config.hpp"
#include "PrototypeBackend.hpp"
#include "ModerationPopup.hpp"
#include "ModerationStore.hpp"

using namespace geode::prelude;

DUDisplaySettingsPopup* DUDisplaySettingsPopup::create() {
    auto ret = new DUDisplaySettingsPopup();
    if (ret && ret->init()) {
        ret->autorelease();
        return ret;
    }
    CC_SAFE_DELETE(ret);
    return nullptr;
}

bool DUDisplaySettingsPopup::init() {
    if (!Popup::init(360.f, 280.f)) return false;
    setTitle("DISPLAY USERNAME");

    m_draft = du::DisplayState::loadDraft();
    if (auto gm = GameManager::sharedState()) m_realName = gm->m_playerName;
    if (m_realName.empty()) m_realName = "Username";

    // Layer-amond balance is always visible in the mod menu.
    if (auto diamond = CCSprite::create("purple-diamond.png"_spr)) {
        diamond->setScale(.085f);
        diamond->setPosition({42.f, 242.f});
        m_mainLayer->addChild(diamond, 3);
    }
    m_balance = CCLabelBMFont::create("0", "bigFont.fnt");
    m_balance->setScale(.40f);
    m_balance->setAnchorPoint({0.f, .5f});
    m_balance->setPosition({56.f, 242.f});
    m_mainLayer->addChild(m_balance, 3);

    auto helpSpr = ButtonSprite::create("?");
    helpSpr->setScale(.48f);
    auto help = CCMenuItemSpriteExtra::create(helpSpr, this, menu_selector(DUDisplaySettingsPopup::onHelp));
    help->setPosition({319.f, 242.f});
    m_buttonMenu->addChild(help);

    if (du::backend::isOwner()) {
        auto modSpr = ButtonSprite::create("MOD");
        modSpr->setScale(.42f);
        auto mod = CCMenuItemSpriteExtra::create(modSpr, this, menu_selector(DUDisplaySettingsPopup::onModeration));
        mod->setPosition({278.f, 242.f});
        m_buttonMenu->addChild(mod);
    }

    auto nameLabel = CCLabelBMFont::create("DISPLAY NAME", "goldFont.fnt");
    nameLabel->setScale(.36f);
    nameLabel->setAnchorPoint({0.f, .5f});
    nameLabel->setPosition({34.f, 215.f});
    m_mainLayer->addChild(nameLabel);

    m_displayInput = TextInput::create(230.f, m_realName, "bigFont.fnt");
    m_displayInput->setPlaceholder(m_realName);
    m_displayInput->setPosition({180.f, 193.f});
    m_displayInput->setMaxCharCount(du::DisplayState::maxCharacters());
    m_displayInput->setString(m_draft.displayName, false);
    m_displayInput->setCallback([this](std::string const& raw) {
        auto sanitized = du::sanitizeDisplayNameInput(raw);
        auto trimmed = du::DisplayState::trimToCodepoints(sanitized, du::DisplayState::maxCharacters());
        if (trimmed != raw && m_displayInput) m_displayInput->setString(trimmed, false);
        m_draft.displayName = trimmed;
        updateCounter();
    });
    m_mainLayer->addChild(m_displayInput);

    m_counter = CCLabelBMFont::create("0/15", "chatFont.fnt");
    m_counter->setScale(.50f);
    m_counter->setAnchorPoint({1.f, .5f});
    m_counter->setPosition({323.f, 173.f});
    m_mainLayer->addChild(m_counter);

    auto shopSpr = ButtonSprite::create("SHOP");
    shopSpr->setScale(.58f);
    auto shop = CCMenuItemSpriteExtra::create(shopSpr, this, menu_selector(DUDisplaySettingsPopup::onShop));
    shop->setPosition({82.f, 151.f});
    m_buttonMenu->addChild(shop);

    auto stickerSpr = ButtonSprite::create("STICKERS");
    stickerSpr->setScale(.58f);
    stickerSpr->setColor(ccc3(125, 125, 125));
    stickerSpr->setOpacity(190);
    m_stickerButton = CCMenuItemSpriteExtra::create(stickerSpr, this, menu_selector(DUDisplaySettingsPopup::onStickers));
    m_stickerButton->setPosition({180.f, 151.f});
    m_buttonMenu->addChild(m_stickerButton);

    if (auto lock = CCSprite::createWithSpriteFrameName("GJ_lockGray_001.png")) {
        lock->setScale(.50f);
        auto cs = stickerSpr->getContentSize();
        lock->setPosition({cs.width * .5f, cs.height * .5f});
        stickerSpr->addChild(lock, 50);
    }

    auto moreSpr = ButtonSprite::create("...");
    moreSpr->setScale(.58f);
    auto more = CCMenuItemSpriteExtra::create(moreSpr, this, menu_selector(DUDisplaySettingsPopup::onMore));
    more->setPosition({278.f, 151.f});
    m_buttonMenu->addChild(more);

    auto displayLabel = CCLabelBMFont::create("DISPLAY VISIBILITY", "goldFont.fnt");
    displayLabel->setScale(.28f);
    displayLabel->setAnchorPoint({0.f, .5f});
    displayLabel->setPosition({34.f, 118.f});
    m_mainLayer->addChild(displayLabel);

    auto displayVisSpr = ButtonSprite::create(du::visibilityLabel(m_draft.displayVisibility));
    displayVisSpr->setScale(.46f);
    m_displayPrivacyButton = CCMenuItemSpriteExtra::create(displayVisSpr, this, menu_selector(DUDisplaySettingsPopup::onDisplayPrivacy));
    m_displayPrivacyButton->setPosition({286.f, 118.f});
    m_buttonMenu->addChild(m_displayPrivacyButton);

    auto stickerLabel = CCLabelBMFont::create("STICKER VISIBILITY", "goldFont.fnt");
    stickerLabel->setScale(.28f);
    stickerLabel->setAnchorPoint({0.f, .5f});
    stickerLabel->setPosition({34.f, 93.f});
    m_mainLayer->addChild(stickerLabel);

    auto stickerVisSpr = ButtonSprite::create(du::visibilityLabel(m_draft.stickerVisibility));
    stickerVisSpr->setScale(.46f);
    m_stickerPrivacyButton = CCMenuItemSpriteExtra::create(stickerVisSpr, this, menu_selector(DUDisplaySettingsPopup::onStickerPrivacy));
    m_stickerPrivacyButton->setPosition({286.f, 93.f});
    m_buttonMenu->addChild(m_stickerPrivacyButton);

    auto showLabel = CCLabelBMFont::create("DEFAULT NAME", "goldFont.fnt");
    showLabel->setScale(.28f);
    showLabel->setAnchorPoint({0.f, .5f});
    showLabel->setPosition({34.f, 68.f});
    m_mainLayer->addChild(showLabel);

    auto showSpr = ButtonSprite::create(m_draft.displayFirst ? "DISPLAY" : "REAL");
    showSpr->setScale(.46f);
    m_showModeButton = CCMenuItemSpriteExtra::create(showSpr, this, menu_selector(DUDisplaySettingsPopup::onShowMode));
    m_showModeButton->setPosition({286.f, 68.f});
    m_buttonMenu->addChild(m_showModeButton);

    auto disableLabel = CCLabelBMFont::create("DISABLE DISPLAY", "goldFont.fnt");
    disableLabel->setScale(.28f);
    disableLabel->setAnchorPoint({0.f, .5f});
    disableLabel->setPosition({34.f, 43.f});
    m_mainLayer->addChild(disableLabel);

    auto disableSpr = ButtonSprite::create(m_draft.displayEnabled ? "OFF" : "ON");
    disableSpr->setScale(.46f);
    m_disableDisplayButton = CCMenuItemSpriteExtra::create(disableSpr, this, menu_selector(DUDisplaySettingsPopup::onDisableDisplay));
    m_disableDisplayButton->setPosition({286.f, 43.f});
    m_buttonMenu->addChild(m_disableDisplayButton);

    auto saveSpr = ButtonSprite::create("SAVE");
    saveSpr->setScale(.58f);
    auto save = CCMenuItemSpriteExtra::create(saveSpr, this, menu_selector(DUDisplaySettingsPopup::onSave));
    save->setPosition({180.f, 17.f});
    m_buttonMenu->addChild(save);

    refresh();
    return true;
}

void DUDisplaySettingsPopup::updateCounter() {
    if (!m_counter) return;
    m_counter->setString(fmt::format(
        "{}/{}",
        du::DisplayState::utf8Length(m_draft.displayName),
        du::DisplayState::maxCharacters()
    ).c_str());
}

void DUDisplaySettingsPopup::updatePrivacyButton(CCMenuItemSpriteExtra* button, du::Visibility visibility) {
    if (!button) return;
    if (auto sprite = typeinfo_cast<ButtonSprite*>(button->getNormalImage())) {
        sprite->setString(du::visibilityLabel(visibility));
    }
}

void DUDisplaySettingsPopup::refresh() {
    if (m_balance) m_balance->setString(fmt::format("{}", du::backend::economy().diamonds).c_str());
    if (m_displayInput) m_displayInput->setMaxCharCount(du::DisplayState::maxCharacters());
    updateCounter();
    updatePrivacyButton(m_displayPrivacyButton, m_draft.displayVisibility);
    updatePrivacyButton(m_stickerPrivacyButton, m_draft.stickerVisibility);

    if (m_showModeButton) {
        if (auto sprite = typeinfo_cast<ButtonSprite*>(m_showModeButton->getNormalImage())) {
            sprite->setString(m_draft.displayFirst ? "DISPLAY" : "REAL");
        }
    }
    if (m_disableDisplayButton) {
        if (auto sprite = typeinfo_cast<ButtonSprite*>(m_disableDisplayButton->getNormalImage())) {
            sprite->setString(m_draft.displayEnabled ? "OFF" : "ON");
        }
    }

    if (m_stickerButton) {
        m_stickerButton->setEnabled(false);
        m_stickerButton->setOpacity(150);
    }
}

void DUDisplaySettingsPopup::onShop(CCObject*) {
    if (auto popup = DUShopPopup::create([this] {
        refresh();
    })) popup->show();
}

void DUDisplaySettingsPopup::onStickers(CCObject*) {
    FLAlertLayer::create("Stickers", "<cy>COMING SOON</c> until Geometry Dash 2.209 support is ready.", "OK")->show();
}

void DUDisplaySettingsPopup::onMore(CCObject*) {
    if (auto popup = DULevelsPopup::create()) popup->show();
}

void DUDisplaySettingsPopup::onDisplayPrivacy(CCObject*) {
    m_draft.displayVisibility = du::nextVisibility(m_draft.displayVisibility);
    updatePrivacyButton(m_displayPrivacyButton, m_draft.displayVisibility);
}

void DUDisplaySettingsPopup::onStickerPrivacy(CCObject*) {
    m_draft.stickerVisibility = du::nextVisibility(m_draft.stickerVisibility);
    updatePrivacyButton(m_stickerPrivacyButton, m_draft.stickerVisibility);
}

void DUDisplaySettingsPopup::onShowMode(CCObject*) {
    m_draft.displayFirst = !m_draft.displayFirst;
    refresh();
}

void DUDisplaySettingsPopup::onDisableDisplay(CCObject*) {
    m_draft.displayEnabled = !m_draft.displayEnabled;
    refresh();
}

void DUDisplaySettingsPopup::onModeration(CCObject*) {
    if (!du::backend::isOwner()) return;
    if (auto popup = DUModerationPopup::create()) popup->show();
}

void DUDisplaySettingsPopup::onHelp(CCObject*) {
    FLAlertLayer::create(
        "Display Username",
        "Set a visual username without changing your real Geometry Dash account name. "
        "Empty field = real name. Latin letters only. Allowed symbols: _ $ - . ! ? '. Emoji, Cyrillic and unusual symbols are not inserted. "
        "In profiles, your real Geometry Dash username stays visible underneath.",
        "OK"
    )->show();
}

void DUDisplaySettingsPopup::onSave(CCObject*) {
    if (m_displayInput) {
        m_draft.displayName = du::DisplayState::trimToCodepoints(
            m_displayInput->getString(), du::DisplayState::maxCharacters()
        );
    }
    auto moderation = du::moderateDisplayName(m_draft.displayName);
    if (!moderation.allowed()) {
        FLAlertLayer::create(
            "Display Username",
            fmt::format("<cr>NAME BLOCKED</c>\n{}", moderation.message).c_str(),
            "OK"
        )->show();
        return;
    }
    if (du::moderation::containsBlockedWord(m_draft.displayName)) {
        FLAlertLayer::create(
            "Display Username",
            "<cr>NAME BLOCKED</c>\nThis display name contains a word blocked by the developer.",
            "OK"
        )->show();
        return;
    }

    du::DisplayState::commitDraft(m_draft);
    // IMPORTANT: publishLocal must only publish a server-approved name once
    // multiplayer sync is enabled. Client-side moderation is bypassable.
    du::sync::publishLocal();
    onClose(nullptr);
}

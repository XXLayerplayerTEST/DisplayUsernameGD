#include "ModerationPopup.hpp"
#include "ModerationStore.hpp"
#include "CoreLogic.hpp"
#include "PrototypeBackend.hpp"

using namespace geode::prelude;

namespace {
constexpr int kPerPage = 5;
}

DUModerationPopup* DUModerationPopup::create() {
    auto ret = new DUModerationPopup();
    if (ret && ret->init()) {
        ret->autorelease();
        return ret;
    }
    CC_SAFE_DELETE(ret);
    return nullptr;
}

bool DUModerationPopup::init() {
    if (!du::backend::isOwner()) return false;
    if (!Popup::init(340.f, 260.f)) return false;
    setTitle("BLOCKED WORDS");

    auto hint = CCLabelBMFont::create("OWNER LIST - MAX 25 CHARS", "goldFont.fnt");
    hint->setScale(.30f);
    hint->setPosition({170.f, 224.f});
    m_mainLayer->addChild(hint);

    m_input = TextInput::create(205.f, "WORD...", "bigFont.fnt");
    m_input->setMaxCharCount(du::moderation::kMaxBlockedWordLength);
    m_input->setPosition({137.f, 195.f});
    m_input->setCallback([this](std::string const& raw) {
        auto clean = du::sanitizeDisplayNameInput(raw);
        clean = du::trimToCodepoints(clean, du::moderation::kMaxBlockedWordLength);
        if (clean != raw && m_input) m_input->setString(clean, false);
    });
    m_mainLayer->addChild(m_input);

    auto addSpr = ButtonSprite::create("+");
    addSpr->setScale(.58f);
    auto add = CCMenuItemSpriteExtra::create(addSpr, this, menu_selector(DUModerationPopup::onAdd));
    add->setPosition({271.f, 195.f});
    m_buttonMenu->addChild(add);

    m_listLayer = CCNode::create();
    m_listLayer->setPosition({0.f, 0.f});
    m_mainLayer->addChild(m_listLayer);

    // Delete buttons must live inside a CCMenu to receive touch/click events.
    // Keeping this menu inside the page layer also means refresh() can clear
    // the whole page cleanly without leaving invisible old buttons behind.
    m_listMenu = CCMenu::create();
    m_listMenu->setPosition({0.f, 0.f});
    m_listLayer->addChild(m_listMenu, 10);

    auto prevSpr = ButtonSprite::create("<");
    prevSpr->setScale(.45f);
    auto prev = CCMenuItemSpriteExtra::create(prevSpr, this, menu_selector(DUModerationPopup::onPrev));
    prev->setPosition({80.f, 24.f});
    m_buttonMenu->addChild(prev);

    auto nextSpr = ButtonSprite::create(">");
    nextSpr->setScale(.45f);
    auto next = CCMenuItemSpriteExtra::create(nextSpr, this, menu_selector(DUModerationPopup::onNext));
    next->setPosition({260.f, 24.f});
    m_buttonMenu->addChild(next);

    m_pageLabel = CCLabelBMFont::create("1/1", "chatFont.fnt");
    m_pageLabel->setScale(.52f);
    m_pageLabel->setPosition({170.f, 24.f});
    m_mainLayer->addChild(m_pageLabel);

    refresh();
    return true;
}

void DUModerationPopup::refresh() {
    m_words = du::moderation::blockedWords();
    int pageCount = std::max(1, static_cast<int>((m_words.size() + kPerPage - 1) / kPerPage));
    m_page = std::clamp(m_page, 0, pageCount - 1);
    if (m_pageLabel) m_pageLabel->setString(fmt::format("{}/{}", m_page + 1, pageCount).c_str());

    if (!m_listLayer) return;
    m_listLayer->removeAllChildren();

    // Recreate the per-page button menu after clearing the list layer.
    // CCMenuItemSpriteExtra does not reliably receive input when parented to a
    // plain CCNode, which is why the old X buttons looked right but did nothing.
    m_listMenu = CCMenu::create();
    m_listMenu->setPosition({0.f, 0.f});
    m_listLayer->addChild(m_listMenu, 10);

    int start = m_page * kPerPage;
    int end = std::min(static_cast<int>(m_words.size()), start + kPerPage);
    for (int i = start; i < end; ++i) {
        int row = i - start;
        // Five roomy rows instead of six cramped rows.
        float y = 158.f - row * 27.f;

        auto bg = CCScale9Sprite::create("square02_001.png");
        bg->setContentSize({258.f, 23.f});
        bg->setOpacity(90);
        bg->setPosition({165.f, y});
        m_listLayer->addChild(bg);

        auto label = CCLabelBMFont::create(m_words[i].c_str(), "bigFont.fnt");
        label->setAnchorPoint({0.f, .5f});
        label->setPosition({44.f, y});
        label->setScale(.32f);
        // Keep long (up to 25-char) entries inside the row and away from X.
        constexpr float kMaxLabelWidth = 205.f;
        float scaledWidth = label->getContentSize().width * label->getScaleX();
        if (scaledWidth > kMaxLabelWidth && label->getContentSize().width > 0.f) {
            label->setScale(kMaxLabelWidth / label->getContentSize().width);
        }
        m_listLayer->addChild(label, 2);

        auto xSpr = ButtonSprite::create("X");
        xSpr->setScale(.38f);
        auto x = CCMenuItemSpriteExtra::create(xSpr, this, menu_selector(DUModerationPopup::onRemove));
        x->setTag(i);
        x->setPosition({289.f, y});
        m_listMenu->addChild(x, 5);
    }

    if (m_words.empty()) {
        auto empty = CCLabelBMFont::create("NO CUSTOM WORDS", "chatFont.fnt");
        empty->setScale(.55f);
        empty->setOpacity(140);
        empty->setPosition({170.f, 110.f});
        m_listLayer->addChild(empty);
    }
}

void DUModerationPopup::onAdd(CCObject*) {
    if (!m_input) return;
    auto word = du::sanitizeDisplayNameInput(m_input->getString());
    word = du::trimToCodepoints(word, du::moderation::kMaxBlockedWordLength);
    if (word.empty()) {
        Notification::create("Enter a word", NotificationIcon::Warning)->show();
        return;
    }
    if (!du::moderation::addBlockedWord(word)) {
        Notification::create("Already in list or invalid", NotificationIcon::Warning)->show();
        return;
    }
    m_input->setString("", false);
    m_words = du::moderation::blockedWords();
    m_page = std::max(0, static_cast<int>((m_words.size() - 1) / kPerPage));
    refresh();
    Notification::create("Blocked word added", NotificationIcon::Success)->show();
}

void DUModerationPopup::onPrev(CCObject*) {
    if (m_page > 0) --m_page;
    refresh();
}

void DUModerationPopup::onNext(CCObject*) {
    int pageCount = std::max(1, static_cast<int>((m_words.size() + kPerPage - 1) / kPerPage));
    if (m_page + 1 < pageCount) ++m_page;
    refresh();
}

void DUModerationPopup::onRemove(CCObject* sender) {
    auto node = typeinfo_cast<CCNode*>(sender);
    if (!node) return;
    int index = node->getTag();
    if (index < 0 || index >= static_cast<int>(m_words.size())) return;
    if (du::moderation::removeBlockedWord(m_words[index])) {
        refresh();
        Notification::create("Blocked word removed", NotificationIcon::Success)->show();
    }
}

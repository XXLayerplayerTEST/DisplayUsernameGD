#include <Geode/Geode.hpp>
#include <Geode/modify/ProfilePage.hpp>
#include <Geode/modify/GJAccountSettingsLayer.hpp>
#include <Geode/modify/CommentCell.hpp>
#include <Geode/modify/LevelInfoLayer.hpp>
#include <Geode/modify/LevelCell.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/LevelBrowserLayer.hpp>
#include <Geode/modify/FLAlertLayer.hpp>
#include <Geode/ui/TextInput.hpp>
#include <Geode/ui/Notification.hpp>

#include "DisplayState.hpp"
#include "DisplaySettingsPopup.hpp"
#include "PrototypeBackend.hpp"
#include "ShopPopup.hpp"
#include "LevelsPopup.hpp"
#include "RatePopup.hpp"
#include "ProfileModerationPopup.hpp"
#include "Sync.hpp"
#include "Config.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cctype>
#include <string>
#include <vector>

using namespace geode::prelude;

namespace {

void fitLabel(CCLabelBMFont* label, float baseScale, float maxWidth);

std::string upper(std::string text) {
    std::transform(text.begin(), text.end(), text.begin(), [](unsigned char c) { return static_cast<char>(std::toupper(c)); });
    return text;
}

CCLabelBMFont* findFirstBMFont(CCNode* node) {
    if (!node) return nullptr;
    if (auto label = typeinfo_cast<CCLabelBMFont*>(node)) return label;
    if (auto children = node->getChildren()) {
        for (auto child : CCArrayExt<CCNode*>(children)) if (auto found = findFirstBMFont(child)) return found;
    }
    return nullptr;
}

CCLabelBMFont* findBMFontByText(CCNode* node, std::string const& text) {
    if (!node) return nullptr;
    if (auto label = typeinfo_cast<CCLabelBMFont*>(node)) if (std::string(label->getString()) == text) return label;
    if (auto children = node->getChildren()) {
        for (auto child : CCArrayExt<CCNode*>(children)) if (auto found = findBMFontByText(child, text)) return found;
    }
    return nullptr;
}

CCLabelBMFont* findBMFontByTextCI(CCNode* node, std::string const& text) {
    if (!node) return nullptr;
    auto wanted = upper(text);
    if (auto label = typeinfo_cast<CCLabelBMFont*>(node)) {
        if (upper(std::string(label->getString())) == wanted) return label;
    }
    if (auto children = node->getChildren()) {
        for (auto child : CCArrayExt<CCNode*>(children)) {
            if (auto found = findBMFontByTextCI(child, text)) return found;
        }
    }
    return nullptr;
}

void collectLabels(CCNode* node, std::vector<CCLabelBMFont*>& out) {
    if (!node) return;
    if (auto label = typeinfo_cast<CCLabelBMFont*>(node)) out.push_back(label);
    if (auto children = node->getChildren()) for (auto child : CCArrayExt<CCNode*>(children)) collectLabels(child, out);
}

void collectNodes(CCNode* node, std::vector<CCNode*>& out) {
    if (!node) return;
    out.push_back(node);
    if (auto children = node->getChildren()) for (auto child : CCArrayExt<CCNode*>(children)) collectNodes(child, out);
}

bool hasAncestorIn(CCNode* node, std::vector<CCNode*> const& candidates);

// Hide only the specific vanilla social row in the lower-right area.
// IMPORTANT: never hide arbitrary nodes just because they share the same Y.
// Earlier builds could accidentally hide the account-settings background or
// profile/settings buttons. This version only hides small row-local nodes.
void hideVanillaSocialRowRight(CCNode* root, std::string const& rowName) {
    if (!root) return;
    std::vector<CCLabelBMFont*> labels;
    collectLabels(root, labels);
    auto wanted = upper(rowName);
    auto rootSize = root->getContentSize();
    float splitX = rootSize.width * .50f;

    for (auto* label : labels) {
        if (!label || !label->getParent()) continue;
        if (upper(std::string(label->getString())).find(wanted) == std::string::npos) continue;

        auto world = label->getParent()->convertToWorldSpace(label->getPosition());
        auto local = root->convertToNodeSpace(world);
        if (local.x < splitX) continue;

        std::vector<CCNode*> all;
        collectNodes(root, all);
        std::vector<CCNode*> candidates;

        for (auto* node : all) {
            if (!node || node == root || !node->getParent() || !node->isVisible()) continue;
            auto wp = node->getParent()->convertToWorldSpace(node->getPosition());
            auto lp = root->convertToNodeSpace(wp);
            auto cs = node->getContentSize();

            // Stay tightly around this social row only.
            if (std::fabs(lp.y - local.y) > 22.f) continue;
            if (lp.x < splitX || lp.x > rootSize.width * .96f) continue;
            if (std::fabs(lp.x - local.x) > 150.f) continue;

            // Never touch large background/panel nodes.
            if (cs.width > 260.f || cs.height > 70.f) continue;
            candidates.push_back(node);
        }

        // Hide only top-level row pieces so children are not processed twice.
        for (auto* node : candidates) {
            if (hasAncestorIn(node, candidates)) continue;
            node->setVisible(false);
        }
    }
}

// Compact the three vanilla permission rows into the upper-right quarter.
// v0.3.7 deliberately touches ONLY the actual permission toggles and their
// text labels. Earlier builds moved arbitrary nearby nodes, which could catch
// unrelated UI/background nodes and break the settings screen.
struct PermissionControlSnapshot {
    CCNode* node = nullptr;
    CCPoint localPos{0.f, 0.f};
    float scaleX = 1.f;
    float scaleY = 1.f;
};

struct PermissionRowSnapshot {
    CCLabelBMFont* title = nullptr;
    CCPoint titleLocal{0.f, 0.f};
    std::vector<PermissionControlSnapshot> controls;
};

bool hasAncestorIn(CCNode* node, std::vector<CCNode*> const& candidates) {
    if (!node) return false;
    auto* p = node->getParent();
    while (p) {
        if (std::find(candidates.begin(), candidates.end(), p) != candidates.end()) return true;
        p = p->getParent();
    }
    return false;
}

bool hasAncestorTypeToggler(CCNode* node) {
    if (!node) return false;
    auto* p = node->getParent();
    while (p) {
        if (typeinfo_cast<CCMenuItemToggler*>(p)) return true;
        p = p->getParent();
    }
    return false;
}

PermissionRowSnapshot capturePermissionRow(CCNode* root, std::string const& heading) {
    PermissionRowSnapshot row;
    if (!root) return row;

    std::vector<CCLabelBMFont*> labels;
    collectLabels(root, labels);
    auto wanted = upper(heading);
    for (auto* label : labels) {
        if (!label || !label->getParent()) continue;
        if (upper(std::string(label->getString())).find(wanted) != std::string::npos) {
            row.title = label;
            break;
        }
    }
    if (!row.title || !row.title->getParent()) return row;

    auto titleWorld = row.title->getParent()->convertToWorldSpace(row.title->getPosition());
    row.titleLocal = root->convertToNodeSpace(titleWorld);
    auto size = root->getContentSize();

    std::vector<CCNode*> all;
    collectNodes(root, all);

    // 1) Capture ONLY togglers belonging to this row.
    std::vector<CCNode*> capturedTogglers;
    for (auto* node : all) {
        auto* toggler = typeinfo_cast<CCMenuItemToggler*>(node);
        if (!toggler || !toggler->getParent() || !toggler->isVisible()) continue;
        auto wp = toggler->getParent()->convertToWorldSpace(toggler->getPosition());
        auto lp = root->convertToNodeSpace(wp);
        if (lp.x < size.width * .05f || lp.x > size.width * .95f) continue;
        if (lp.y < row.titleLocal.y - 31.f || lp.y > row.titleLocal.y + 4.f) continue;
        capturedTogglers.push_back(toggler);
        row.controls.push_back({toggler, lp, toggler->getScaleX(), toggler->getScaleY()});
    }

    // 2) Capture option labels (ALL / FRIENDS / NONE / ME) only when they are
    // not already children of a toggler. This avoids moving anything twice.
    static const std::vector<std::string> kOptionTexts = {"ALL", "FRIENDS", "NONE", "ME"};
    for (auto* label : labels) {
        if (!label || label == row.title || !label->getParent() || !label->isVisible()) continue;
        auto text = upper(std::string(label->getString()));
        if (std::find(kOptionTexts.begin(), kOptionTexts.end(), text) == kOptionTexts.end()) continue;
        if (hasAncestorTypeToggler(label)) continue;

        auto wp = label->getParent()->convertToWorldSpace(label->getPosition());
        auto lp = root->convertToNodeSpace(wp);
        if (lp.x < size.width * .05f || lp.x > size.width * .95f) continue;
        if (lp.y < row.titleLocal.y - 31.f || lp.y > row.titleLocal.y + 4.f) continue;
        row.controls.push_back({label, lp, label->getScaleX(), label->getScaleY()});
    }

    return row;
}

void applyPermissionRow(CCNode* root, PermissionRowSnapshot const& row, float targetX, float targetY, float targetWidth) {
    if (!root || !row.title || !row.title->getParent()) return;

    // The target point is the centre of one of the three native brown social
    // input strips. We only move the real permission title/toggles into that
    // strip; no backgrounds, parents, or unrelated nodes are moved.
    auto titleWorld = root->convertToWorldSpace({targetX, targetY + 6.f});
    row.title->setPosition(row.title->getParent()->convertToNodeSpace(titleWorld));
    row.title->setAnchorPoint({.5f,.5f});
    fitLabel(row.title, .25f, std::max(120.f, targetWidth * .88f));

    if (row.controls.empty()) return;

    float minX = row.controls.front().localPos.x;
    float maxX = row.controls.front().localPos.x;
    float minY = row.controls.front().localPos.y;
    float maxY = row.controls.front().localPos.y;
    for (auto const& snap : row.controls) {
        minX = std::min(minX, snap.localPos.x);
        maxX = std::max(maxX, snap.localPos.x);
        minY = std::min(minY, snap.localPos.y);
        maxY = std::max(maxY, snap.localPos.y);
    }

    float sourceCX = (minX + maxX) * .5f;
    float sourceCY = (minY + maxY) * .5f;
    float sourceWidth = std::max(1.f, maxX - minX);
    float desiredWidth = std::max(95.f, targetWidth * .78f);
    float compact = std::clamp(desiredWidth / sourceWidth, .40f, .58f);

    for (auto const& snap : row.controls) {
        if (!snap.node || !snap.node->getParent()) continue;
        float nx = targetX + (snap.localPos.x - sourceCX) * compact;
        float ny = targetY - 7.f + (snap.localPos.y - sourceCY) * compact;
        auto wp = root->convertToWorldSpace({nx, ny});
        snap.node->setPosition(snap.node->getParent()->convertToNodeSpace(wp));
        snap.node->setScaleX(snap.scaleX * compact);
        snap.node->setScaleY(snap.scaleY * compact);
    }
}

struct NativeSocialRow {
    CCLabelBMFont* title = nullptr;
    CCTextInputNode* input = nullptr;
    CCPoint titleLocal{0.f, 0.f};
    CCPoint inputLocal{0.f, 0.f};
    bool titleVisible = true;
    bool inputVisible = true;
    std::string initialText;
};

NativeSocialRow captureNativeSocialRow(CCNode* root, std::string const& heading) {
    NativeSocialRow out;
    if (!root) return out;

    std::vector<CCLabelBMFont*> labels;
    collectLabels(root, labels);
    auto wanted = upper(heading);
    for (auto* label : labels) {
        if (!label || !label->getParent()) continue;
        auto text = upper(std::string(label->getString()));
        if (text.find(wanted) == std::string::npos) continue;
        out.title = label;
        auto world = label->getParent()->convertToWorldSpace(label->getPosition());
        out.titleLocal = root->convertToNodeSpace(world);
        out.titleVisible = label->isVisible();
        break;
    }
    if (!out.title) return out;

    // Find only the actual native text input that belongs to this social row.
    // Do NOT gather arbitrary nearby nodes. The old proximity-based approach
    // accidentally moved large input/backdrop nodes and produced the huge dark
    // rectangles seen in v0.4.1.
    std::vector<CCNode*> all;
    collectNodes(root, all);
    float bestScore = 1e9f;
    for (auto* node : all) {
        auto* input = typeinfo_cast<CCTextInputNode*>(node);
        if (!input || !input->getParent()) continue;
        auto world = input->getParent()->convertToWorldSpace(input->getPosition());
        auto local = root->convertToNodeSpace(world);
        float dx = std::fabs(local.x - out.titleLocal.x);
        float dy = std::fabs(local.y - out.titleLocal.y);
        if (dy > 38.f || dx > 230.f) continue;
        float score = dy * 4.f + dx;
        if (score < bestScore) {
            bestScore = score;
            out.input = input;
            out.inputLocal = local;
        }
    }

    if (out.input) {
        out.inputVisible = out.input->isVisible();
        out.initialText = out.input->getString();
    }
    return out;
}

void setNativeSocialRowVisible(NativeSocialRow const& row, bool visible) {
    if (row.title) row.title->setVisible(visible && row.titleVisible);
    if (row.input) row.input->setVisible(visible && row.inputVisible);
}

void turnNativeSocialRowIntoStrip(NativeSocialRow const& row) {
    // Keep the native input box itself visible so its brown field becomes the
    // strip requested by the user, but remove its old social heading/value.
    if (row.title) row.title->setVisible(false);
    if (row.input) {
        row.input->setVisible(row.inputVisible);
        row.input->setString("");
    }
}

CCMenuItemSpriteExtra* findButtonByText(CCNode* node, std::string const& wanted) {
    if (!node) return nullptr;
    if (auto item = typeinfo_cast<CCMenuItemSpriteExtra*>(node)) {
        if (auto normal = item->getNormalImage()) {
            if (auto label = findFirstBMFont(normal)) if (upper(std::string(label->getString())) == upper(wanted)) return item;
        }
    }
    if (auto children = node->getChildren()) for (auto child : CCArrayExt<CCNode*>(children)) if (auto found = findButtonByText(child, wanted)) return found;
    return nullptr;
}


bool containsTextRecursive(CCNode* node, std::string const& wanted) {
    if (!node) return false;
    auto needle = upper(wanted);
    if (auto bm = typeinfo_cast<CCLabelBMFont*>(node)) {
        if (upper(std::string(bm->getString())).find(needle) != std::string::npos) return true;
    }
    if (auto ttf = typeinfo_cast<CCLabelTTF*>(node)) {
        if (upper(std::string(ttf->getString())).find(needle) != std::string::npos) return true;
    }
    if (auto children = node->getChildren()) {
        for (auto child : CCArrayExt<CCNode*>(children)) if (containsTextRecursive(child, wanted)) return true;
    }
    return false;
}

void fitLabel(CCLabelBMFont* label, float baseScale, float maxWidth) {
    if (!label) return;
    label->setScale(baseScale);
    auto width = label->getContentSize().width;
    if (width > 0.f) label->setScale(std::max(.28f, std::min(baseScale, maxWidth / width)));
}

struct NamesToRender {
    std::string primary;
    std::string secondary;
    bool primaryIsDisplay = false;
    bool secondaryIsDisplay = false;
};

ccColor3B displayNameBlue() {
    return ccc3(70, 170, 255);
}

NamesToRender namesForSelf(std::string const& real) {
    if (!du::DisplayState::displayEnabled()) return {real, {}, false, false};
    auto display = du::DisplayState::effectiveName(real);
    if (display == real) return {real, {}, false, false};
    return du::DisplayState::displayFirst()
        ? NamesToRender{display, real, true, false}
        : NamesToRender{real, display, false, true};
}

NamesToRender namesForRemote(std::string const& real, du::sync::RemoteProfile const& remote) {
    auto display = real;
    if (remote.displayEnabled && remote.displayAllowed && !remote.displayName.empty() &&
        du::moderateDisplayName(remote.displayName).allowed()) {
        display = remote.displayName;
    }
    if (display == real) return {real, {}, false, false};
    return remote.displayFirst
        ? NamesToRender{display, real, true, false}
        : NamesToRender{real, display, false, true};
}

void addSecondaryName(
    CCNode* parent,
    CCLabelBMFont* original,
    std::string const& smallName,
    std::string const& id,
    bool isDisplay,
    ccColor3B const& normalColor
) {
    if (!parent || !original) return;
    if (auto old = parent->getChildByID(id)) old->removeFromParent();
    if (smallName.empty()) return;
    auto secondaryLabel = CCLabelBMFont::create(smallName.c_str(), "chatFont.fnt");
    secondaryLabel->setID(id);
    secondaryLabel->setAnchorPoint({.5f, .5f});
    secondaryLabel->setScale(.48f);
    secondaryLabel->setOpacity(205);
    secondaryLabel->setColor(isDisplay ? displayNameBlue() : normalColor);

    // Position the secondary line from the primary label's WORLD position.
    // This keeps it centered below the name even when the vanilla username
    // lives inside a transformed child/menu node.
    auto world = original->getParent()->convertToWorldSpace(original->getPosition());
    auto local = parent->convertToNodeSpace(world);
    secondaryLabel->setPosition({local.x, local.y - 13.f});
    fitLabel(secondaryLabel, .48f, 165.f);
    parent->addChild(secondaryLabel, original->getZOrder() + 1);
}

// Account settings stay vanilla: no original node is moved, hidden, resized,
// recolored, or replaced. We only add one independent Display button in the corner.
class $modify(DUAccountSettingsButton, GJAccountSettingsLayer) {
    bool init(int accountID) {
        if (!GJAccountSettingsLayer::init(accountID)) return false;
        if (!m_mainLayer) return true;

        auto size = m_mainLayer->getContentSize();
        auto menu = CCMenu::create();
        menu->setID("display-username-settings-menu"_spr);
        menu->setPosition({0.f, 0.f});
        m_mainLayer->addChild(menu, 100);

        auto icon = CCSprite::create("display-button.png"_spr);
        if (!icon) return true;
        icon->setScale(.70f);

        auto button = CCMenuItemSpriteExtra::create(
            icon, this, menu_selector(DUAccountSettingsButton::onDisplaySettings)
        );
        button->setID("display-username-settings-button"_spr);
        button->setPosition({size.width - 52.f, size.height - 52.f});
        menu->addChild(button);
        return true;
    }

    void onDisplaySettings(CCObject*) {
        if (auto popup = DUDisplaySettingsPopup::create()) popup->show();
    }
};

class $modify(DUProfilePage, ProfilePage) {
    struct Fields {
        CCLabelBMFont* originalLabel = nullptr;
        std::string realName;
        std::string displayName;
        bool hasDisplay = false;
        float baseScale = 1.f;
        ccColor3B baseColor = ccc3(255,255,255);
        int targetAccountID = 0;
        std::string targetUserName;
    };

    void clearDUProfileNodes() {
        if (!m_mainLayer) return;
        static const std::array<std::string, 2> ids = {
            "du-profile-display-name"_spr,
            "du-profile-real-name"_spr,
        };
        for (auto const& id : ids) {
            if (auto node = m_mainLayer->getChildByID(id)) node->removeFromParent();
        }
    }

    void clearDUOwnerMenu() {
        if (!m_mainLayer) return;
        if (auto old = m_mainLayer->getChildByID("du-owner-profile-menu"_spr)) old->removeFromParent();
    }

    void renderOwnerModerationButtons(bool self) {
        if (!m_mainLayer || self || !du::backend::isOwner() || m_fields->targetAccountID <= 0) return;
        if (m_fields->targetAccountID == du::config::kOwnerAccountID) return;

        clearDUOwnerMenu();

        auto size = m_mainLayer->getContentSize();
        auto menu = CCMenu::create();
        menu->setID("du-owner-profile-menu"_spr);
        menu->setPosition({0.f, 0.f});
        m_mainLayer->addChild(menu, 120);

        auto banSpr = ButtonSprite::create(du::backend::isUserBanned(m_fields->targetAccountID) ? "UNBAN" : "BAN");
        banSpr->setScale(.42f);
        auto ban = CCMenuItemSpriteExtra::create(banSpr, this, menu_selector(DUProfilePage::onDUProfileBan));
        ban->setPosition({size.width * .5f - 58.f, size.height - 68.f});
        menu->addChild(ban);

        auto displaySpr = ButtonSprite::create("DISPLAY");
        displaySpr->setScale(.42f);
        auto display = CCMenuItemSpriteExtra::create(displaySpr, this, menu_selector(DUProfilePage::onDUProfileDisplay));
        display->setPosition({size.width * .5f + 58.f, size.height - 68.f});
        menu->addChild(display);
    }

    void onDUProfileBan(CCObject*) {
        if (!du::backend::isOwner() || m_fields->targetAccountID <= 0) return;
        if (du::backend::isUserBanned(m_fields->targetAccountID)) {
            if (du::backend::unbanUser(m_fields->targetAccountID)) {
                Notification::create("User unbanned", NotificationIcon::Success)->show();
                renderOwnerModerationButtons(false);
            }
            return;
        }
        if (auto popup = DUBanUserPopup::create(m_fields->targetAccountID, m_fields->targetUserName)) popup->show();
    }

    void onDUProfileDisplay(CCObject*) {
        if (!du::backend::isOwner() || m_fields->targetAccountID <= 0) return;
        if (auto popup = DUChangeDisplayPopup::create(m_fields->targetAccountID, m_fields->targetUserName)) popup->show();
    }

    void renderStableProfileNames() {
        if (!m_mainLayer || !m_fields->originalLabel) return;
        clearDUProfileNodes();

        if (!m_fields->hasDisplay) {
            m_fields->originalLabel->setVisible(true);
            return;
        }

        // Do not fight Geometry Dash' username menu layout every frame. Hide the
        // vanilla username only when a Display Username exists and draw our two
        // lines directly on the stable ProfilePage main layer.
        m_fields->originalLabel->setVisible(false);
        auto size = m_mainLayer->getContentSize();
        CCPoint top{size.width * .5f, size.height - 34.f};

        auto display = CCLabelBMFont::create(m_fields->displayName.c_str(), "bigFont.fnt");
        display->setID("du-profile-display-name"_spr);
        display->setAnchorPoint({.5f,.5f});
        display->setPosition(top);
        display->setColor(displayNameBlue());
        fitLabel(display, m_fields->baseScale, 190.f);
        m_mainLayer->addChild(display, 60);

        auto real = CCLabelBMFont::create(m_fields->realName.c_str(), "chatFont.fnt");
        real->setID("du-profile-real-name"_spr);
        real->setAnchorPoint({.5f,.5f});
        real->setPosition({top.x, top.y - 14.f});
        real->setColor(m_fields->baseColor);
        real->setOpacity(220);
        fitLabel(real, .50f, 175.f);
        m_mainLayer->addChild(real, 60);
    }

    void loadPageFromUserInfo(GJUserScore* score) {
        ProfilePage::loadPageFromUserInfo(score);
        if (!score || !m_mainLayer) return;

        if (m_fields->originalLabel) m_fields->originalLabel->setVisible(true);
        clearDUProfileNodes();
        clearDUOwnerMenu();

        auto real = std::string(score->m_userName);
        NamesToRender names{real, {}, false, false};
        auto account = GJAccountManager::sharedState();
        bool self = account && score->m_accountID == account->m_accountID;

        m_fields->targetAccountID = score->m_accountID;
        m_fields->targetUserName = real;
        renderOwnerModerationButtons(self);

        if (self) {
            names = namesForSelf(real);
        } else if (du::backend::isUserBanned(score->m_accountID)) {
            names = {real, {}, false, false};
        } else if (auto forced = du::backend::userDisplayOverride(score->m_accountID)) {
            names = {*forced, real, true, false};
        } else if (auto remote = du::sync::getCached(score->m_accountID)) {
            names = namesForRemote(real,*remote);
        }

        auto menu = typeinfo_cast<CCMenu*>(m_mainLayer->getChildByIDRecursive("username-menu"));
        if (!menu) {
            return;
        }

        auto original = findBMFontByText(menu, real);
        if (!original) original = findBMFontByTextCI(menu, real);
        if (!original) original = findBMFontByText(m_mainLayer, real);
        if (!original) original = findBMFontByTextCI(m_mainLayer, real);
        if (!original) {
            return;
        }

        m_fields->originalLabel = original;
        m_fields->realName = real;
        m_fields->baseScale = original->getScale();
        m_fields->baseColor = original->getColor();
        m_fields->hasDisplay = names.primaryIsDisplay || names.secondaryIsDisplay;
        m_fields->displayName.clear();

        if (m_fields->hasDisplay) {
            m_fields->displayName = names.primaryIsDisplay ? names.primary : names.secondary;
            if (m_fields->displayName.empty()) m_fields->displayName = du::DisplayState::effectiveName(real);
        }

        renderStableProfileNames();
    }
};

class $modify(DUCommentCell, CommentCell) {
    struct Fields {
        CCMenu* usernameMenu = nullptr;
        CCLabelBMFont* primaryLabel = nullptr;
        std::string realName;
        std::string displayName;
        bool hasDisplay = false;
        bool configuredDisplayFirst = true;
        float baseScale = 1.f;
        CCPoint basePosition{0.f, 0.f};
        CCPoint baseAnchor{.5f, .5f};
        ccColor3B baseColor = ccc3(255,255,255);
    };

    void renderCommentName(bool realFirst) {
        if (!m_fields->primaryLabel || !m_fields->usernameMenu || !m_fields->hasDisplay) return;
        auto primary = realFirst ? m_fields->realName : m_fields->displayName;
        bool primaryIsDisplay = !realFirst;
        m_fields->primaryLabel->setString(primary.c_str());
        m_fields->primaryLabel->setColor(primaryIsDisplay ? displayNameBlue() : m_fields->baseColor);
        fitLabel(m_fields->primaryLabel, m_fields->baseScale, 118.f);
        if (auto old = m_fields->usernameMenu->getChildByID("secondary-username-comment"_spr)) old->removeFromParent();
        if (auto old = m_fields->usernameMenu->getChildByID("display-name-comment-hitbox"_spr)) old->removeFromParent();
    }

    void loadFromComment(GJComment* comment) {
        CommentCell::loadFromComment(comment);
        if (!comment || !m_mainLayer) return;
        auto real = std::string(comment->m_userName);
        NamesToRender names{real, {}, false, false};
        auto account = GJAccountManager::sharedState();
        bool self = account && comment->m_accountID == account->m_accountID;
        if (self) {
            names = namesForSelf(real);
        } else if (du::backend::isUserBanned(comment->m_accountID)) {
            names = {real, {}, false, false};
        } else if (auto forced = du::backend::userDisplayOverride(comment->m_accountID)) {
            names = {*forced, real, true, false};
        } else if (auto remote = du::sync::getCached(comment->m_accountID)) {
            names = namesForRemote(real,*remote);
        }
        auto menu = typeinfo_cast<CCMenu*>(m_mainLayer->getChildByIDRecursive("username-menu"));
        if (!menu) return;
        auto original = findBMFontByText(menu, real);
        if (!original) return;

        m_fields->usernameMenu = menu;
        m_fields->primaryLabel = original;
        m_fields->realName = real;
        m_fields->baseScale = original->getScale();
        m_fields->basePosition = original->getPosition();
        m_fields->baseAnchor = original->getAnchorPoint();
        m_fields->baseColor = original->getColor();
        m_fields->hasDisplay = names.primaryIsDisplay || names.secondaryIsDisplay;
        m_fields->configuredDisplayFirst = names.primaryIsDisplay;

        if (!m_fields->hasDisplay) {
            original->setString(real.c_str());
            original->setAnchorPoint(m_fields->baseAnchor);
            original->setPosition(m_fields->basePosition);
            original->setColor(m_fields->baseColor);
            fitLabel(original, m_fields->baseScale, 118.f);
            if (auto old = menu->getChildByID("secondary-username-comment"_spr)) old->removeFromParent();
            if (auto old = menu->getChildByID("display-name-comment-hitbox"_spr)) old->removeFromParent();
            return;
        }

        m_fields->displayName = names.primaryIsDisplay ? names.primary : names.secondary;
        // No hover/tap swapping. Comments simply use the configured default.
        renderCommentName(!m_fields->configuredDisplayFirst);
    }
};

class $modify(DULevelCell, LevelCell) {
    void loadFromLevel(GJGameLevel* level) {
        LevelCell::loadFromLevel(level);
        if (!level) return;

        if (auto old = this->getChildByID("du-cell-reward"_spr)) old->removeFromParent();
        if (auto old = this->getChildByID("du-cell-feature"_spr)) old->removeFromParent();
        if (auto old = this->getChildByID("du-cell-diamond"_spr)) old->removeFromParent();

        auto rated = du::backend::ratedLevel(level->m_levelID.value());
        if (!rated) return;

        auto size = getContentSize();
        float x = size.width - 78.f;
        float y = 18.f;

        auto diamond = CCSprite::create("purple-diamond.png"_spr);
        if (diamond) {
            diamond->setID("du-cell-diamond"_spr);
            diamond->setScale(.048f);
            diamond->setPosition({x - 28.f, y});
            addChild(diamond, 25);
        }

        bool claimed = du::backend::isBaseClaimed(level->m_levelID.value());
        auto rewardText = claimed ? fmt::format("{} CLAIMED", rated->reward) : fmt::format("{}", rated->reward);
        auto reward = CCLabelBMFont::create(rewardText.c_str(), claimed ? "goldFont.fnt" : "bigFont.fnt");
        reward->setID("du-cell-reward"_spr);
        reward->setScale(.24f);
        reward->setAnchorPoint({0.f,.5f});
        reward->setPosition({x - 19.f, y});
        addChild(reward, 25);

        auto feature = CCLabelBMFont::create(
            fmt::format("DU {}", du::featureLabel(rated->feature)).c_str(),
            "chatFont.fnt"
        );
        feature->setID("du-cell-feature"_spr);
        feature->setScale(.27f);
        feature->setAnchorPoint({1.f,.5f});
        feature->setPosition({size.width - 13.f, y + 18.f});
        addChild(feature, 25);
    }
};

class $modify(DULevelInfoLayer, LevelInfoLayer) {
    struct Fields { GJGameLevel* level = nullptr; };

    bool init(GJGameLevel* level, bool challenge) {
        if (!LevelInfoLayer::init(level, challenge)) return false;
        m_fields->level = level;

        if (level) {
            if (auto rated = du::backend::ratedLevel(level->m_levelID.value())) {
                CCPoint base{88.f, 172.f};
                if (m_difficultySprite && m_difficultySprite->getParent()) {
                    auto wp = m_difficultySprite->getParent()->convertToWorldSpace(m_difficultySprite->getPosition());
                    base = this->convertToNodeSpace(wp);
                }

                // DU Opinion uses Geometry Dash's own difficulty / feature sprites.
                // Keep it on the LEFT of the vanilla difficulty so it never overlaps PLAY.
                auto difficultyFrame = [&]() -> char const* {
                    switch (rated->difficulty) {
                        case du::Difficulty::NA: return "difficulty_00_btn_001.png";
                        case du::Difficulty::Easy: return "difficulty_01_btn_001.png";
                        case du::Difficulty::Normal: return "difficulty_02_btn_001.png";
                        case du::Difficulty::Hard: return "difficulty_03_btn_001.png";
                        case du::Difficulty::Harder: return "difficulty_04_btn_001.png";
                        case du::Difficulty::Insane: return "difficulty_05_btn_001.png";
                        case du::Difficulty::Auto: return "difficulty_auto_btn_001.png";
                        case du::Difficulty::Demon:
                            switch (rated->demonType) {
                                case du::DemonType::Easy: return "difficulty_07_btn_001.png";
                                case du::DemonType::Medium: return "difficulty_08_btn_001.png";
                                case du::DemonType::Hard: return "difficulty_06_btn_001.png";
                                case du::DemonType::Insane: return "difficulty_09_btn_001.png";
                                case du::DemonType::Extreme: return "difficulty_10_btn_001.png";
                            }
                    }
                    return "difficulty_00_btn_001.png";
                }();

                float awayFromPlay = base.x < getContentSize().width * .5f ? -1.f : 1.f;
                CCPoint opinionPos{base.x + awayFromPlay * 66.f, base.y};
                auto opinionTitle = CCLabelBMFont::create("DU OPINION", "goldFont.fnt");
                opinionTitle->setScale(.20f);
                opinionTitle->setPosition({opinionPos.x, opinionPos.y + 31.f});
                addChild(opinionTitle, 1000);

                if (auto opinionIcon = CCSprite::createWithSpriteFrameName(difficultyFrame)) {
                    opinionIcon->setScale(.72f);
                    opinionIcon->setPosition(opinionPos);
                    addChild(opinionIcon, 1000);
                }

                char const* featureFrame = nullptr;
                switch (rated->feature) {
                    case du::FeatureTier::None: break;
                    case du::FeatureTier::Featured: featureFrame = "GJ_featuredCoin_001.png"; break;
                    case du::FeatureTier::Epic: featureFrame = "GJ_epicCoin_001.png"; break;
                    case du::FeatureTier::Legendary: featureFrame = "GJ_epicCoin2_001.png"; break;
                    case du::FeatureTier::Mythic: featureFrame = "GJ_epicCoin3_001.png"; break;
                }
                if (featureFrame) {
                    if (auto featureIcon = CCSprite::createWithSpriteFrameName(featureFrame)) {
                        featureIcon->setScale(.36f);
                        featureIcon->setPosition({opinionPos.x + awayFromPlay * 31.f, opinionPos.y - 2.f});
                        addChild(featureIcon, 1001);
                    }
                }

                float rewardY = base.y - 58.f;
                auto diamond = CCSprite::create("purple-diamond.png"_spr);
                if (diamond) {
                    diamond->setScale(.085f);
                    diamond->setPosition({base.x - 18.f, rewardY});
                    addChild(diamond, 1000);
                }

                bool baseClaimed = du::backend::isBaseClaimed(level->m_levelID.value());
                auto rewardText = baseClaimed
                    ? fmt::format("{}  CLAIMED", rated->reward)
                    : fmt::format("{}", rated->reward);
                auto reward = CCLabelBMFont::create(rewardText.c_str(), baseClaimed ? "goldFont.fnt" : "bigFont.fnt");
                reward->setScale(.36f);
                reward->setAnchorPoint({0.f,.5f});
                reward->setPosition({base.x - 7.f, rewardY});
                addChild(reward, 1000);

                if (rated->daily) {
                    bool dailyClaimed = du::backend::isDailyClaimed(rated->dailyGeneration);
                    auto bonusText = dailyClaimed
                        ? fmt::format("DAILY BONUS +{}  CLAIMED", rated->dailyBonus)
                        : fmt::format("DAILY BONUS +{}", rated->dailyBonus);
                    auto bonus = CCLabelBMFont::create(bonusText.c_str(), dailyClaimed ? "goldFont.fnt" : "bigFont.fnt");
                    bonus->setScale(.27f);
                    bonus->setColor(ccc3(225, 80, 255));
                    bonus->setPosition({base.x + 20.f, rewardY - 10.f});
                    addChild(bonus, 1000);
                }
            }
        }

        if (!du::backend::isOwner()) return true;
        auto menu = CCMenu::create();
        menu->setPosition({0,0});
        menu->setID("du-rate-menu"_spr);
        addChild(menu,999);
        auto spr = ButtonSprite::create("R");
        spr->setScale(.65f);
        auto btn = CCMenuItemSpriteExtra::create(spr,this,menu_selector(DULevelInfoLayer::onRatePanel));
        auto win = CCDirector::sharedDirector()->getWinSize();
        btn->setPosition({62.f,win.height-86.f});
        menu->addChild(btn);
        return true;
    }

    void onRatePanel(CCObject*) {
        if (m_fields->level) if (auto p = DURatePopup::create(m_fields->level)) p->show();
    }
};

// DAILY / VIEW first opens an exact-ID online search, then this hook jumps
// straight to the level page when the result arrives.
class $modify(DULevelBrowserLayer, LevelBrowserLayer) {
    void loadLevelsFinished(CCArray* levels, char const* key, int type) {
        LevelBrowserLayer::loadLevelsFinished(levels, key, type);
        int pending = Mod::get()->getSavedValue<int>("du-pending-level-open", 0);
        if (pending <= 0 || !levels) return;
        for (auto obj : CCArrayExt<CCObject*>(levels)) {
            auto level = typeinfo_cast<GJGameLevel*>(obj);
            if (!level || level->m_levelID.value() != pending) continue;
            Mod::get()->setSavedValue("du-pending-level-open", 0);
            GameLevelManager::sharedState()->gotoLevelPage(level);
            break;
        }
    }
};

class $modify(DUPlayLayer, PlayLayer) {
    void levelComplete() {
        int levelID = m_level ? m_level->m_levelID.value() : 0;
        PlayLayer::levelComplete();
        if (levelID <= 0) return;
        auto claim = du::backend::claimCompletion(levelID);
        if (claim.total > 0) {
            auto msg = claim.dailyBonus > 0 ? fmt::format("+{} Layer-amonds ({} + {} Daily)",claim.total,claim.base,claim.dailyBonus) : fmt::format("+{} Layer-amonds",claim.total);
            Notification::create(msg,NotificationIcon::Success)->show();
        }
    }
};

} // namespace

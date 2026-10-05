#pragma once

#include <Geode/Geode.hpp>
#include <Geode/ui/Popup.hpp>
#include <Geode/ui/TextInput.hpp>
#include <string>
#include <vector>

class DUModerationPopup : public geode::Popup {
protected:
    bool init();
    void refresh();
    void onAdd(cocos2d::CCObject*);
    void onPrev(cocos2d::CCObject*);
    void onNext(cocos2d::CCObject*);
    void onRemove(cocos2d::CCObject*);

    geode::TextInput* m_input = nullptr;
    cocos2d::CCLabelBMFont* m_pageLabel = nullptr;
    cocos2d::CCNode* m_listLayer = nullptr;
    cocos2d::CCMenu* m_listMenu = nullptr;
    int m_page = 0;
    std::vector<std::string> m_words;

public:
    static DUModerationPopup* create();
};

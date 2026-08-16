#include "loadpackwidget.h"
#include "../uilib/hbox.h"
#include "../uilib/vbox.h"
#include "../uilib/scrollvbox.h"
#include "../uilib/label.h"
#include "defaults.h" // DEFAULT_FONT_*

namespace Ui {

LoadPackWidget::LoadPackWidget(int x, int y, int w, int h, FontStore *fontStore)
        : SimpleContainer(x,y,w,h), _fontStore(fontStore)
{
    const auto ui = getNativeUiMetrics();
    _font = _fontStore->getFont(DEFAULT_FONT_NAME, ui.fontSize);
    _smallFont = _fontStore->getFont(DEFAULT_FONT_NAME, std::max(1, ui.fontSize - 2));
    if (_font && !_smallFont) _smallFont = _font;

    onClick += {this, [this](void*, int, int, int button) {
        if (button == MouseButton::BUTTON_RIGHT)
            setVisible(false);
    }};

    _packs = new ScrollVBox(0,0,0,0);
    _packs->setGrow(1,1);
    _packs->setPadding(0);
    _packs->setSpacing(ui.panelSpacing);

    _variants = new ScrollVBox(0,0,0,0);
    _variants->setGrow(1,1);
    _variants->setPadding(0);
    _variants->setSpacing(ui.panelSpacing);

#ifdef __ANDROID__
    // The desktop picker uses two narrow columns and hover to select a pack.
    // Android gets a single-column, two-page picker with explicit navigation.
    _mobileHeader = new HBox(0,0,0,ui.touchTarget);
    _mobileHeader->setPadding(0);
    _mobileHeader->setSpacing(ui.toolbarSpacing);

    _mobileBack = new Label(0,0,ui.touchTarget,ui.touchTarget,_font,"<");
    _mobileBack->setMinSize({ui.touchTarget,ui.touchTarget});
    _mobileBack->setMaxSize({ui.touchTarget,ui.touchTarget});
    _mobileBack->setBackground({255,255,255,22});
    _mobileBack->setCornerRadius(ui.toolbarCornerRadius);
    _mobileBack->setTextAlignment(Label::HAlign::CENTER, Label::VAlign::MIDDLE);
    _mobileBack->onClick += {this, [this](void*, int, int, int button) {
        if (button == MouseButton::BUTTON_LEFT)
            showPackList();
    }};

    _mobileTitle = new Label(0,0,0,ui.touchTarget,_font,"Choose tracker");
    _mobileTitle->setGrow(1,1);
    _mobileTitle->setMinSize({0,ui.touchTarget});
    _mobileTitle->setTextAlignment(Label::HAlign::CENTER, Label::VAlign::MIDDLE);

    _mobileClose = new Label(0,0,ui.touchTarget,ui.touchTarget,_font,"X");
    _mobileClose->setMinSize({ui.touchTarget,ui.touchTarget});
    _mobileClose->setMaxSize({ui.touchTarget,ui.touchTarget});
    _mobileClose->setBackground({255,255,255,22});
    _mobileClose->setCornerRadius(ui.toolbarCornerRadius);
    _mobileClose->setTextAlignment(Label::HAlign::CENTER, Label::VAlign::MIDDLE);
    _mobileClose->onClick += {this, [this](void*, int, int, int button) {
        if (button == MouseButton::BUTTON_LEFT)
            setVisible(false);
    }};

    _mobileHeader->addChild(_mobileBack);
    _mobileHeader->addChild(_mobileTitle);
    _mobileHeader->addChild(_mobileClose);
    addChild(_mobileHeader);
    addChild(_packs);
    addChild(_variants);
    showPackList();
#else
    auto hbox = new HBox(0,0,0,0);
    hbox->setGrow(1,1);
    hbox->addChild(_packs);
    hbox->addChild(_variants);
    hbox->setSpacing(ui.panelSpacing);
    hbox->setPadding(ui.panelPadding);
    addChild(hbox);
    _main = hbox;
#endif
}

#ifdef __ANDROID__
void LoadPackWidget::showPackList()
{
    _mobileTitle->setText("Choose tracker");
    _mobileBack->setVisible(false);
    _packs->setVisible(true);
    _variants->setVisible(false);
    layoutMobile();
}

void LoadPackWidget::showVariants(const Pack::Info& pack)
{
    const auto ui = getNativeUiMetrics();
    _variants->clearChildren();
    _curVariantLabel = nullptr;

    for (const auto& variant : pack.variants) {
        auto* lbl = new Label(0,0,_variants->getWidth(),ui.packRowHeight,
            _font,"  " + variant.name);
        lbl->setGrow(1,0);
        lbl->setMinSize({0,ui.packRowHeight});
        lbl->setBackground(VARIANT_BG_DEFAULT);
        lbl->setCornerRadius(ui.toolbarCornerRadius);
        lbl->setTextAlignment(Label::HAlign::LEFT, Label::VAlign::MIDDLE);

        lbl->onMouseEnter += {this, [this](void* s, int, int, unsigned) {
            if (_curVariantLabel && _curVariantLabel != s)
                _curVariantLabel->setBackground(VARIANT_BG_DEFAULT);
            _curVariantLabel = static_cast<Label*>(s);
            _curVariantLabel->setBackground(VARIANT_BG_HOVER);
        }};
        lbl->onMouseLeave += {this, [this](void* s) {
            if (_curVariantLabel != s) return;
            _curVariantLabel->setBackground(VARIANT_BG_DEFAULT);
            _curVariantLabel = nullptr;
        }};
        const auto path = pack.path;
        lbl->onClick += {this, [this,path,variant](void*, int, int, int button) {
            if (button == MouseButton::BUTTON_LEFT)
                onPackSelected.emit(this,path,variant.variant);
        }};
        _variants->addChild(lbl);
    }

    _mobileTitle->setText("Choose variant");
    _mobileBack->setVisible(true);
    _packs->setVisible(false);
    _variants->setVisible(true);
    layoutMobile();
}

void LoadPackWidget::layoutMobile()
{
    if (!_mobileHeader) return;
    const auto ui = getNativeUiMetrics();
    const int padding = ui.panelPadding;
    const int contentTop = padding + ui.touchTarget + ui.toolbarSpacing;
    const int contentWidth = std::max(1, _size.width - 2 * padding);
    const int contentHeight = std::max(1, _size.height - contentTop - padding);

    _mobileHeader->setPosition({padding,padding});
    _mobileHeader->setSize({contentWidth,ui.touchTarget});
    _packs->setPosition({padding,contentTop});
    _packs->setSize({contentWidth,contentHeight});
    _variants->setPosition({padding,contentTop});
    _variants->setSize({contentWidth,contentHeight});
}
#endif

void LoadPackWidget::update()
{
    const auto ui = getNativeUiMetrics();
    const auto availablePacks = Pack::ListAvailable();
    _packs->clearChildren();
    _variants->clearChildren();
    _curPackHover = nullptr;
    _curPackLabel = nullptr;
    _curVariantLabel = nullptr;
    _disableHoverSelect = false;

    for (const auto& pack : availablePacks) {
#ifdef __ANDROID__
        std::string text = "  " + pack.packName;
        if (!pack.version.empty())
            text += "  " + pack.version;
        auto* lbl = new Label(0,0,_packs->getWidth(),ui.packRowHeight,_font,text);
        lbl->setGrow(1,0);
        lbl->setMinSize({0,ui.packRowHeight});
        lbl->setTextAlignment(Label::HAlign::LEFT, Label::VAlign::MIDDLE);
        lbl->setBackground(PACK_BG_DEFAULT);
        lbl->setCornerRadius(ui.toolbarCornerRadius);
        lbl->onMouseEnter += {this, [this](void* s, int, int, unsigned) {
            if (_curPackHover && _curPackHover != s)
                _curPackHover->setBackground(PACK_BG_DEFAULT);
            _curPackHover = static_cast<Label*>(s);
            _curPackHover->setBackground(PACK_BG_HOVER);
        }};
        lbl->onMouseLeave += {this, [this](void* s) {
            if (_curPackHover != s) return;
            _curPackHover->setBackground(PACK_BG_DEFAULT);
            _curPackHover = nullptr;
        }};
        lbl->onClick += {this, [this,pack](void*, int, int, int button) {
            if (button == MouseButton::BUTTON_LEFT)
                showVariants(pack);
        }};
        _packs->addChild(lbl);
#else
        auto* lbl = new Label(0,0,0,0,_font," " + pack.packName + " " + pack.version);
        lbl->setGrow(1,0);
        lbl->setTextAlignment(Label::HAlign::LEFT, Label::VAlign::MIDDLE);
        lbl->setMinSize({64,ui.packRowHeight});
        lbl->setSize({_size.width/2,ui.packRowHeight});
        lbl->setBackground(PACK_BG_DEFAULT);
        _packs->addChild(lbl);

        lbl->onMouseEnter += {this,[this,pack,ui](void* s, int, int, unsigned) {
            if (_curPackHover != s) {
                if (_curPackHover == _curPackLabel && _disableHoverSelect)
                    _curPackHover->setBackground(PACK_BG_ACTIVE);
                else if (_curPackHover)
                    _curPackHover->setBackground(PACK_BG_DEFAULT);

                _curPackHover = static_cast<Label*>(s);
                if (_curPackHover == _curPackLabel && _disableHoverSelect)
                    _curPackHover->setBackground(PACK_BG_ACTIVE_HOVER);
                else
                    _curPackHover->setBackground(PACK_BG_HOVER);
            }
            if (_disableHoverSelect || _curPackLabel == s)
                return;

            if (_curPackLabel)
                _curPackLabel->setBackground(PACK_BG_DEFAULT);

            _curPackLabel = static_cast<Label*>(s);
            _variants->clearChildren();
            for (const auto& variant: pack.variants) {
                auto* variantLabel = new Label(0,0,0,0,_font," " + variant.name);
                variantLabel->setGrow(1,0);
                variantLabel->setTextAlignment(Label::HAlign::LEFT, Label::VAlign::MIDDLE);
                variantLabel->setMinSize({64,ui.packRowHeight});
                variantLabel->setSize({_variants->getWidth(),ui.packRowHeight});
                variantLabel->setBackground(VARIANT_BG_DEFAULT);
                _variants->addChild(variantLabel);
                variantLabel->onMouseLeave += {this,[this](void* child) {
                    if (_curVariantLabel != child) return;
                    _curVariantLabel->setBackground(VARIANT_BG_DEFAULT);
                    _curVariantLabel = nullptr;
                }};
                const auto path = pack.path;
                variantLabel->onMouseEnter += {this,[this](void* child, int, int, unsigned) {
                    if (!child || child == _curVariantLabel) return;
                    if (_curVariantLabel)
                        _curVariantLabel->onMouseLeave.emit(_curVariantLabel);
                    _curVariantLabel = static_cast<Label*>(child);
                    _curVariantLabel->setBackground(VARIANT_BG_HOVER);
                }};
                variantLabel->onClick += {this,[this,path,variant](void*, int, int, int button) {
                    if (button == MouseButton::BUTTON_LEFT)
                        onPackSelected.emit(this,path,variant.variant);
                }};
            }
            _main->relayout();
            _variants->relayout();
            _main->relayout();
        }};

        lbl->onClick += {this, [this](void* s, int x, int y, int buttons) {
            _disableHoverSelect = false;
            static_cast<Label*>(s)->onMouseEnter.emit(s,x,y,static_cast<unsigned>(buttons));
            _disableHoverSelect = true;
            if (_curPackLabel)
                _curPackLabel->setBackground(PACK_BG_ACTIVE_HOVER);
        }};
#endif
    }

#ifndef __ANDROID__
    _packs->onMouseLeave += {this, [this](void*) {
        if (_disableHoverSelect && _curPackHover) {
            if (_curPackHover == _curPackLabel)
                _curPackHover->setBackground(PACK_BG_ACTIVE);
            else
                _curPackHover->setBackground(PACK_BG_DEFAULT);
        }
    }};
#endif

    if (availablePacks.empty()) {
#ifdef __ANDROID__
        auto* lbl = new Label(0,0,_packs->getWidth(),ui.packRowHeight * 2,_font,
            "No tracker packs installed.\nUse ZIP+ in the toolbar to import one.");
        lbl->setMinSize({0,ui.packRowHeight * 2});
        lbl->setTextAlignment(Label::HAlign::CENTER, Label::VAlign::MIDDLE);
        _packs->addChild(lbl);
#else
        auto* lbl = new Label(0,0,0,0,_font,
            "No packs installed!\nDrag & drop packs into the window to install them.");
        lbl->setGrow(1,1);
        lbl->setTextAlignment(Label::HAlign::LEFT, Label::VAlign::MIDDLE);
        lbl->setMinSize(lbl->getAutoSize());
        lbl->setSize({_size.width/2,ui.packRowHeight});
        _packs->addChild(lbl);
        auto* spacer = new Label(0,0,0,0,nullptr,"");
        spacer->setGrow(1,1);
        _packs->addChild(spacer);
#endif
    }

#ifdef __ANDROID__
    showPackList();
#else
    _main->setSize(_size);
    _main->relayout();
#endif
}

void LoadPackWidget::setSize(Size size)
{
    SimpleContainer::setSize(size);
#ifdef __ANDROID__
    layoutMobile();
#else
    _packs->setWidth(size.width/2-1);
    _variants->setWidth(size.width/2-1);
    _main->relayout();
#endif
}

} // namespace Ui

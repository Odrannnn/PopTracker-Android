#include "defaulttrackerwindow.h"
#include "../uilib/dlg.h"
#include "../uilib/hbox.h"
#include "../core/assets.h"
#include <cmath>
#include <cstdio>

namespace Ui {

DefaultTrackerWindow::DefaultTrackerWindow(const char* title, SDL_Surface* icon, const Position& pos, const Size& size, const WindowConfig& config)
    : TrackerWindow(title, icon, pos, size, config)
{
    const auto ui = getNativeUiMetrics();
    auto hbox = new HBox(0,0,_size.width,ui.toolbarHeight);
    hbox->setBackground({18,18,18,255});
    hbox->setPadding(ui.toolbarPadding);
    hbox->setSpacing(ui.toolbarSpacing);
    hbox->setGrow(1,1);
    _menu = hbox;
    addChild(_menu);

#ifdef __ANDROID__
    _mobileSectionBar = new HFlexBox(
        0, ui.toolbarHeight, _size.width, ui.touchTarget,
        HFlexBox::HAlign::CENTER
    );
    _mobileSectionBar->setPadding(ui.toolbarPadding);
    _mobileSectionBar->setSpacing(ui.toolbarSpacing);
    _mobileSectionBar->setBackground({24,24,24,255});
    _mobileSectionBar->setVisible(false);
    addChild(_mobileSectionBar);
#endif
    
    _btnLoad = new ImageButton(0,0,ui.touchTarget,ui.touchTarget, asset("load.png"));
    _btnLoad->setDarkenGreyscale(false);
    hbox->addChild(_btnLoad);
    _btnLoad->onClick += { this, [this](void*, int, int, int) {
        onMenuPressed.emit(this, MENU_LOAD, 0);
    }};

#ifdef __ANDROID__
    _btnImportPack = new Label(0,0,ui.touchTarget,ui.touchTarget,_font,"ZIP+");
    _btnImportPack->setTextAlignment(Label::HAlign::CENTER, Label::VAlign::MIDDLE);
    _btnImportPack->setBackground({255,255,255,22});
    _btnImportPack->setCornerRadius(ui.toolbarCornerRadius);
    hbox->addChild(_btnImportPack);
    _btnImportPack->onClick += { this, [this](void*, int, int, int) {
        onMenuPressed.emit(this, MENU_IMPORT_PACK, 0);
    }};
    _btnImportPack->onMouseEnter += {this, [this](void*, int, int, unsigned) {
        setTooltip("Import Tracker ZIP");
    }};
    _btnImportPack->onMouseLeave += {this, [this](void*) {
        setTooltip("");
    }};

    _btnWorkspaceZoom = new Label(0,0,ui.touchTarget,ui.touchTarget,_font,"1x");
    _btnWorkspaceZoom->setTextAlignment(Label::HAlign::CENTER, Label::VAlign::MIDDLE);
    _btnWorkspaceZoom->setBackground({255,255,255,22});
    _btnWorkspaceZoom->setCornerRadius(ui.toolbarCornerRadius);
    _btnWorkspaceZoom->setVisible(false);
    hbox->addChild(_btnWorkspaceZoom);
    _btnWorkspaceZoom->onClick += {this, [this](void*, int, int, int button) {
        if (!_view)
            return;
        if (button == BUTTON_RIGHT)
            _view->resetWorkspaceView();
        else if (button == BUTTON_LEFT)
            _view->cycleWorkspaceZoom();
    }};
    _btnWorkspaceZoom->onMouseEnter += {this, [this](void*, int, int, unsigned) {
        setTooltip("Tap to zoom tracker; hold to fit");
    }};
    _btnWorkspaceZoom->onMouseLeave += {this, [this](void*) {
        setTooltip("");
    }};
#endif
    
    _btnReload = new ImageButton(0,0,ui.touchTarget,ui.touchTarget, asset("reload.png"));
    _btnReload->setVisible(false);
    hbox->addChild(_btnReload);
    _btnReload->onClick += { this, [this](void*, int, int, int) {
        onMenuPressed.emit(this, MENU_RELOAD, 0);
    }};
    
    _btnImport = new ImageButton(0,0,ui.touchTarget,ui.touchTarget, asset("import.png"));
    _btnImport->setVisible(false);
    hbox->addChild(_btnImport);
    _btnImport->onClick += { this, [this](void*, int, int, int) {
        onMenuPressed.emit(this, MENU_LOAD_STATE, 0);
    }};

    _btnExport = new ImageButton(0,0,ui.touchTarget,ui.touchTarget, asset("export.png"));
    _btnExport->setVisible(false);
    hbox->addChild(_btnExport);
    _btnExport->onClick += { this, [this](void*, int, int, int) {
        onMenuPressed.emit(this, MENU_SAVE_STATE, 0);
    }};

#if !defined(__EMSCRIPTEN__) && !defined(__ANDROID__) // no multi-window support
    _btnBroadcast = new ImageButton(0,0,ui.touchTarget,ui.touchTarget, asset("broadcast.png"));
    _btnBroadcast->setVisible(false);
    hbox->addChild(_btnBroadcast);
    _btnBroadcast->onClick += { this, [this](void*, int, int, int) {
        onMenuPressed.emit(this, MENU_BROADCAST, 0);
    }};
#endif

    if (config.showAlwaysOnTopButton()) {
        _btnAlwaysOnTop = new ImageButton(0,0,ui.touchTarget,ui.touchTarget, asset("always_on_top.png"));
        hbox->addChild(_btnAlwaysOnTop);
        _btnAlwaysOnTop->onClick += { this, [this](void *s, int, int, int) {
            ImageButton* btn = (ImageButton*)s;
            btn->setEnabled(!btn->getEnabled()); // toggle greyscale
            onMenuPressed.emit(this, MENU_ALWAYS_ON_TOP, 0);
        }};
    }
    
#ifndef __ANDROID__
    _btnPackSettings = new ImageButton(ui.touchTarget,0,ui.touchTarget,ui.touchTarget, asset("settings.png"));
    _btnPackSettings->setVisible(false);
    hbox->addChild(_btnPackSettings);
    _btnPackSettings->onClick += { this, [this](void*, int, int, int) {
        onMenuPressed.emit(this, MENU_PACK_SETTINGS, 0);
    }};
#endif

    _hboxAutoTrackers = new HBox(0,0,0,ui.touchTarget);
    _hboxAutoTrackers->setPadding(0);
    _hboxAutoTrackers->setSpacing(ui.toolbarSpacing);
    hbox->addChild(_hboxAutoTrackers);

#ifdef __ANDROID__
    auto tooltipFont = _fontStore->getFont(DEFAULT_FONT_NAME, ui.tooltipFontSize);
    _lblTooltip = new Label(ui.overlayMargin,
        ui.toolbarHeight + ui.overlayMargin,
        std::max(1, _size.width - 2 * ui.overlayMargin),
        ui.tooltipHeight, tooltipFont, "");
    _lblTooltip->setBackground({24,24,24,235});
    _lblTooltip->setCornerRadius(ui.overlayMargin);
    _lblTooltip->setTextAlignment(Label::HAlign::CENTER, Label::VAlign::MIDDLE);
    _lblTooltip->setVisible(false);
    addChild(_lblTooltip);
#else
    _lblTooltip = new Label(0,0,0,0,_font,"");
    _lblTooltip->setHeight(ui.touchTarget);
    _lblTooltip->setGrow(1,1);
    _lblTooltip->setTextAlignment(Label::HAlign::RIGHT, Label::VAlign::MIDDLE);
    hbox->addChild(_lblTooltip);
#endif

    _lblMessage = new Label(0,ui.toolbarHeight + ui.toolbarSpacing,_size.width,0, _font, "");
    _lblMessage->setVisible(false);
    _lblMessage->setTextAlignment(Label::HAlign::CENTER, Label::VAlign::TOP);
    addChild(_lblMessage);

    if (!Dlg::hasGUI()) {
        showMessage("WARNING: Dialog helper not available. Certain things will be broken!\n"
                "Please install which and zenity, kdialog, matedialog, qarma, xdialog or python + tkinter.",
                true);
    }

    _loadPackWidget = new LoadPackWidget(0,0,0,0,_fontStore);
#ifdef __ANDROID__
    _loadPackWidget->setPosition({0,ui.toolbarHeight});
    _loadPackWidget->setSize({_size.width,std::max(1,_size.height-ui.toolbarHeight)});
#else
    _loadPackWidget->setPosition({0,0});
    _loadPackWidget->setSize(_size);
#endif
    _loadPackWidget->setBackground({0,0,0});
    _loadPackWidget->setVisible(false);
    addChild(_loadPackWidget);
    
    _loadPackWidget->onPackSelected += {this,[this](void *, const fs::path& pack, const std::string& variant) {
        onPackSelected.emit(this,pack,variant);
    }};
    
    for (const auto& pair : {
            std::pair{_btnLoad, "Load Pack"},
            std::pair{_btnReload, "Reload Pack"},
            std::pair{_btnImport, "Import State"},
            std::pair{_btnExport, "Export State"},
            std::pair{_btnBroadcast, "Open Broadcast View"},
            std::pair{_btnPackSettings, "Open Pack Settings"}
    }) {
        if (!pair.first) continue;
        pair.first->setDarkenGreyscale(false);
#ifdef __ANDROID__
        pair.first->setEnabled(true);
        pair.first->setQuality(2);
        pair.first->setContentPadding(std::max(0,
            (ui.touchTarget - ui.toolbarIconSize) / 2));
        pair.first->setCornerRadius(ui.toolbarCornerRadius);
        pair.first->setBackground({255,255,255,22});
#else
        pair.first->setEnabled(false); // make greyscale
#endif
        const char* text = pair.second; // pointer to program memory
        pair.first->onMouseEnter += {this, [this,text](void *s, int, int, unsigned)
        {
            ImageButton* btn = (ImageButton*)s;
            btn->setEnabled(true); // disable greyscale
            setTooltip(text);
        }};
        pair.first->onMouseLeave += {this, [this](void *s)
        {
            ImageButton* btn = (ImageButton*)s;
            btn->setEnabled(false); // enable greyscale
            setTooltip("");
        }};
    }

    if (_btnAlwaysOnTop) {
        _btnAlwaysOnTop->setDarkenGreyscale(false);
#ifdef __ANDROID__
        _btnAlwaysOnTop->setQuality(2);
        _btnAlwaysOnTop->setContentPadding(std::max(0,
            (ui.touchTarget - ui.toolbarIconSize) / 2));
        _btnAlwaysOnTop->setCornerRadius(ui.toolbarCornerRadius);
        _btnAlwaysOnTop->setBackground({255,255,255,22});
#endif
        _btnAlwaysOnTop->onMouseEnter += {this, [this](void*, int, int, unsigned)
        {
            setTooltip("Toggle Always On Top");
        }};
        _btnAlwaysOnTop->onMouseLeave += {this, [this](void*)
        {
            setTooltip("");
        }};
    }
}

DefaultTrackerWindow::~DefaultTrackerWindow()
{
    // TODO: make sure widgets are deleted in container's destructor or delete here
    _btnLoad = nullptr;
    _btnImportPack = nullptr;
    _btnWorkspaceZoom = nullptr;
    _mobileSectionBar = nullptr;
    _mobileSectionButtons.clear();
    _mobileSections.clear();
    _btnReload = nullptr;
    _btnBroadcast = nullptr;
    _btnAlwaysOnTop = nullptr;
    _btnPackSettings = nullptr;
    _hboxAutoTrackers = nullptr;
    _lblsAutoTrackers.clear();
    _vboxProgress = nullptr;
    _lblProgressTitle = nullptr;
    _lblProgressPercent = nullptr;
    _lblProgressValues = nullptr;
    _pgbProgress = nullptr;
}

void DefaultTrackerWindow::setTracker(Tracker* tracker)
{
    std::string layout = _aspectRatio > 1 ? "tracker_horizontal" : _aspectRatio < 1 ? "tracker_vertical" : "tracker_default";
    if (tracker && !tracker->hasLayout(layout)) {
        if (tracker->hasLayout("tracker_default")) layout = "tracker_default";
        else if (tracker->hasLayout("tracker_horizontal")) layout = "tracker_horizontal";
        else if (tracker->hasLayout("tracker_vertical")) layout = "tracker_vertical";
    }
    setTracker(tracker, layout);
}

void DefaultTrackerWindow::setTracker(Tracker* tracker, const std::string& layout)
{
    if (_view && _view->getTracker() == tracker && _view->getLayoutRoot() == layout) {
#ifdef __ANDROID__
        // Packs can register their root layout before layouts referenced by it.
        // onLayoutChanged calls back into setTracker as each file is loaded, so
        // refresh the derived mobile navigation even when the selected root did
        // not change.
        rebuildMobileSections(tracker, layout);
#endif
        return;
    }

    TrackerWindow::setTracker(tracker, layout);
    rebuildMobileSections(tracker, layout);
    if (tracker) {
#ifdef __ANDROID__
        if (_btnWorkspaceZoom) {
            _btnWorkspaceZoom->setVisible(true);
            _view->onWorkspaceZoomChanged += {this, [this](void*, float zoom) {
                updateWorkspaceZoomLabel(zoom);
            }};
            updateWorkspaceZoomLabel(_view->getWorkspaceZoom());
        }
#endif
        hideMessage();
        tracker->onLayoutChanged -= this;
        tracker->onLayoutChanged += {this, [this,tracker](void*, const std::string&) {
            if (_btnBroadcast) _btnBroadcast->setVisible(tracker->hasLayout("tracker_broadcast"));
            if (_btnPackSettings) _btnPackSettings->setVisible(tracker->hasLayout("settings_popup"));
            setTracker(tracker); // reevaluate preferred and fall-back layout
        }};
        _view->onItemHover += {this, [this, tracker](void*, const std::string& itemid) {
            if (!_lastHoverItem.empty()) {
                auto& item = tracker->getItemById(itemid);
                item.onChange -= this;
            }
            if (itemid.empty()) setTooltip("");
            else {
                auto& item = tracker->getItemById(itemid);
                const auto& text = (item.getBaseItem().empty() || item.getState()) ?
                    item.getCurrentName() : tracker->getItemByCode(item.getBaseItem()).getCurrentName();
                setTooltip(text);
                item.onChange += {this, [this, tracker, itemid](void*) {
                    const auto& item = tracker->getItemById(itemid);
                    const auto& text = (item.getBaseItem().empty() || item.getState()) ?
                        item.getCurrentName() : tracker->getItemByCode(item.getBaseItem()).getCurrentName();
                    setTooltip(text);
                }};
            }
            _lastHoverItem = itemid;
        }};
        if (_btnReload) _btnReload->setVisible(true);
        if (_btnImport) _btnImport->setVisible(true);
        if (_btnExport) _btnExport->setVisible(true);
    } else {
        if (_btnBroadcast) _btnBroadcast->setVisible(false);
        if (_btnPackSettings) _btnPackSettings->setVisible(false);
        if (_btnReload) _btnReload->setVisible(false);
        if (_btnImport) _btnImport->setVisible(false);
        if (_btnExport) _btnExport->setVisible(false);
#ifdef __ANDROID__
        if (_btnWorkspaceZoom) _btnWorkspaceZoom->setVisible(false);
#endif
    }
    raiseChild(_loadPackWidget);
}

void DefaultTrackerWindow::rebuildMobileSections(
    Tracker* tracker, const std::string& layout
)
{
#ifdef __ANDROID__
    if (!_mobileSectionBar)
        return;
    std::string previousTitle;
    if (_activeMobileSection >= 0 &&
            static_cast<size_t>(_activeMobileSection) < _mobileSections.size())
        previousTitle = _mobileSections[_activeMobileSection].title;

    _mobileSectionButtons.clear();
    _mobileSectionBar->clearChildren();
    _mobileSections = TrackerView::discoverMobileSections(tracker, layout);
    _activeMobileSection = -1;

    if (!_view || _mobileSections.size() < 2) {
        _mobileSectionBar->setVisible(false);
        if (_view) _view->setMobileSection(nullptr);
        updateMobileSectionLayout(_size);
        return;
    }

    const auto ui = getNativeUiMetrics();
    _mobileSectionBar->setVisible(true);
    _mobileSectionBar->setWidth(std::max(1, _size.width));
    for (size_t index = 0; index < _mobileSections.size(); index++) {
        const auto& section = _mobileSections[index];
        auto* button = new Button(0, 0, 0, ui.touchTarget, _font, section.title);
        button->setSize({
            std::max(ui.touchTarget, button->getAutoWidth()),
            ui.touchTarget,
        });
        button->setMinimumHitSize({ui.touchTarget, ui.touchTarget});
        button->setCornerRadius(ui.toolbarCornerRadius);
        button->setState(Button::State::NORMAL);
        button->onClick += {this, [this,index](void*, int, int, int button) {
            if (button == BUTTON_LEFT)
                selectMobileSection(index);
        }};
        _mobileSectionButtons.push_back(button);
        _mobileSectionBar->addChild(button);
    }

    size_t selected = 0;
    if (!previousTitle.empty()) {
        for (size_t index = 0; index < _mobileSections.size(); index++) {
            if (_mobileSections[index].title == previousTitle) {
                selected = index;
                break;
            }
        }
    }
    updateMobileSectionLayout(_size);
    selectMobileSection(selected);
#else
    (void)tracker;
    (void)layout;
#endif
}

void DefaultTrackerWindow::selectMobileSection(size_t index)
{
#ifdef __ANDROID__
    if (!_view || index >= _mobileSections.size())
        return;
    _activeMobileSection = static_cast<int>(index);
    for (size_t buttonIndex = 0;
            buttonIndex < _mobileSectionButtons.size(); buttonIndex++) {
        _mobileSectionButtons[buttonIndex]->setState(
            buttonIndex == index ? Button::State::PRESSED : Button::State::NORMAL
        );
    }
    _view->setMobileSection(&_mobileSections[index]);
#else
    (void)index;
#endif
}

int DefaultTrackerWindow::updateMobileSectionLayout(Size size)
{
    const auto ui = getNativeUiMetrics();
    int contentTop = ui.toolbarHeight;
#ifdef __ANDROID__
    if (_mobileSectionBar && _mobileSectionBar->getVisible()) {
        _mobileSectionBar->setPosition({0, ui.toolbarHeight});
        _mobileSectionBar->setSize({std::max(1, size.width), ui.touchTarget});
        contentTop = _mobileSectionBar->getTop() + _mobileSectionBar->getHeight();
    }
    if (_view) {
        _view->setPosition({0, contentTop});
        _view->setSize({
            std::max(1, size.width),
            std::max(1, size.height - contentTop),
        });
    }
#else
    (void)size;
#endif
    return contentTop;
}

void DefaultTrackerWindow::updateWorkspaceZoomLabel(float zoom)
{
#ifdef __ANDROID__
    if (!_btnWorkspaceZoom)
        return;
    char text[12];
    if (std::abs(zoom - std::round(zoom)) < 0.05f)
        std::snprintf(text, sizeof(text), "%dx", static_cast<int>(std::round(zoom)));
    else
        std::snprintf(text, sizeof(text), "%.1fx", zoom);
    _btnWorkspaceZoom->setText(text);
#else
    (void)zoom;
#endif
}

void DefaultTrackerWindow::showMessage(const std::string& message, bool error)
{
    _lblMessage->setText(message);
    _lblMessage->setHeight(_lblMessage->getAutoHeight());
    if (error)
        _lblMessage->setTextColor({0xff, 0x7f, 0x7f, 0xff});
    else
        _lblMessage->setTextColor({0xff, 0xff, 0xff, 0xff});
    _lblMessage->setVisible(true);
}

void DefaultTrackerWindow::hideMessage()
{
    _lblMessage->setVisible(false);
}

void DefaultTrackerWindow::setTooltip(const std::string& text)
{
    _lblTooltip->setText(text);
#ifdef __ANDROID__
    _lblTooltip->setVisible(!text.empty());
    if (!text.empty()) {
        _tooltipShownAt = getTicks();
        raiseChild(_lblTooltip);
    }
#endif
}

void DefaultTrackerWindow::render(Renderer renderer, int offX, int offY)
{
#ifdef __ANDROID__
    if (_lblTooltip->getVisible() && elapsed(_tooltipShownAt, 3000))
        _lblTooltip->setVisible(false);
#endif
    if (_view) {
        float oldAspectRatio = _aspectRatio;
        _aspectRatio = (float)getWidth() / (float)getHeight();
        Tracker* tracker = _view->getTracker();
        if (tracker && ((_aspectRatio > 1 && oldAspectRatio <= 1) || (_aspectRatio < 1 && oldAspectRatio >= 1)))
            setTracker(tracker); // reevaluate preferred and fall-back layout
    }
    TrackerWindow::render(renderer, offX, offY);
}

void DefaultTrackerWindow::setAutoTrackerState(int index, AutoTracker::State state, const std::string& name, const std::string& subname)
{
    const auto ui = getNativeUiMetrics();
    if (index<0) {
        // clear all labels
        for (size_t i=0; i<_lblsAutoTrackers.size(); i++) {
            _autoTrackerStates[i] = state;
            auto lbl = _lblsAutoTrackers[i];
            lbl->setText(name);
            lbl->setWidth(std::max(lbl->getAutoWidth(), ui.touchTarget));
        }
        if (!_lblsAutoTrackers.empty()) {
            _hboxAutoTrackers->relayout();
            auto menu = dynamic_cast<HBox*>(_menu);
            if (menu) menu->relayout();
        }
        return;
    }

    Label* lbl = nullptr;
    if (_lblsAutoTrackers.size() > (size_t)index) {
        lbl = _lblsAutoTrackers[index];
        auto oldWidth = lbl->getWidth();
        lbl->setText(name);
        lbl->setWidth(std::max(lbl->getAutoWidth(), ui.touchTarget));
        if (lbl->getWidth() != oldWidth) {
            _hboxAutoTrackers->relayout();
            auto menu = dynamic_cast<HBox*>(_menu);
            if (menu) menu->relayout();
        }
        _autoTrackerStates[index] = state;
        _autoTrackerNames[index] = name;
        _autoTrackerSubNames[index] = subname;
    } else {
        printf("adding label #%d \"%s\"\n", index, name.c_str());
        lbl = new Label(0,0,0,ui.touchTarget,_font, name);
        lbl->setWidth(std::max(lbl->getAutoWidth(), ui.touchTarget));
        lbl->setTextColor({0,0,0});
#ifdef __ANDROID__
        lbl->setBackground({255,255,255,22});
        lbl->setCornerRadius(ui.touchTarget / 2);
#endif
        lbl->onClick += {this, [this,index](void*, int, int, int button) {
            if (button == BUTTON_RIGHT) {
                onMenuPressed.emit(this, MENU_CYCLE_AUTOTRACKER, index);
            } else {
                onMenuPressed.emit(this, MENU_TOGGLE_AUTOTRACKER, index);
            }
        }};
        lbl->onMouseEnter += {this, [this,index](void*, int, int, unsigned)
        {
            auto state = _autoTrackerStates[index];
            const char* text = state == AutoTracker::State::Disconnected ? "Offline" :
                               state == AutoTracker::State::BridgeConnected ? "No Game/Console" :
                               state == AutoTracker::State::ConsoleConnected ? "Online" :
                               state == AutoTracker::State::Disabled ? "Disabled" :
                               state == AutoTracker::State::Unavailable ? nullptr : "Unknown";
            std::string name = _autoTrackerNames[index];
            if (name.empty()) name = "Auto Tracker";
            if (state == AutoTracker::State::ConsoleConnected) {
                if (!_autoTrackerSubNames[index].empty())
                    name = _autoTrackerSubNames[index];
            }
            if (text) {
                setTooltip(name + ": " + text);
            }
        }};
        lbl->onMouseLeave += {this, [this](void*)
        {
            setTooltip("");
        }};
        _lblsAutoTrackers.push_back(lbl);
        _autoTrackerStates.push_back(state);
        _autoTrackerNames.push_back(name);
        _autoTrackerSubNames.push_back(subname);
        _hboxAutoTrackers->addChild(lbl);
        _hboxAutoTrackers->relayout();
        auto menu = dynamic_cast<HBox*>(_menu);
        if (menu) menu->relayout();
    }

    lbl->setTextAlignment(Label::HAlign::CENTER, Label::VAlign::MIDDLE);
    if (state == AutoTracker::State::Disconnected) {
        lbl->setTextColor({255,0,0});
    } else if (state == AutoTracker::State::BridgeConnected) {
        lbl->setTextColor({255,255,0});
    } else if (state == AutoTracker::State::ConsoleConnected) {
        lbl->setTextColor({0,255,0});
    } else if (state == AutoTracker::State::Disabled) {
        lbl->setTextColor({128,128,128});
    } else if (state == AutoTracker::State::Unavailable) {
        lbl->setTextColor({0,0,0}); // hidden
    }

    if (isHover(lbl)) {
        // update tool tip if current tool tip is AT state
        lbl->onMouseEnter.emit(lbl, 0, 0, 0);
    }
}

void DefaultTrackerWindow::setSize(Size size)
{
    TrackerWindow::setSize(size);
    _lblMessage->setWidth(size.width);
#ifdef __ANDROID__
    const auto ui = getNativeUiMetrics();
    const int contentTop = updateMobileSectionLayout(size);
    _lblTooltip->setPosition({ui.overlayMargin, contentTop + ui.overlayMargin});
    _lblTooltip->setSize({std::max(1, size.width - 2 * ui.overlayMargin), ui.tooltipHeight});
#endif
#ifdef __ANDROID__
    _loadPackWidget->setPosition({0,ui.toolbarHeight});
    _loadPackWidget->setSize({size.width,std::max(1,size.height-ui.toolbarHeight)});
#else
    _loadPackWidget->setSize(size);
#endif
    if (_vboxProgress) {
        _vboxProgress->setPosition({
            (_size.width - _vboxProgress->getWidth())/2,
            (_size.height - _vboxProgress->getHeight())/2
        });
    }
}

void DefaultTrackerWindow::setMinSize(const Size size)
{
    TrackerWindow::setMinSize(size || Size{220, 0});
}

void DefaultTrackerWindow::showOpen()
{
    _loadPackWidget->update();
    _loadPackWidget->setVisible(true);
    raiseChild(_loadPackWidget);
}
void DefaultTrackerWindow::hideOpen()
{
    _loadPackWidget->setVisible(false);
}

void DefaultTrackerWindow::showProgress(const std::string& text, int progress, int max)
{
    char percentText[5];
    if (max>0) snprintf(percentText, 5, "%3u%%", upercent(progress, max));
    else percentText[0] = 0;
    std::string valuesText = format_bytes(progress) + "B";
    if (max) valuesText += " / " + format_bytes(max) + "B";

    if (!_vboxProgress) {
        int w = 300;
        int h = 69; // FIXME: should be able to auto-size this
        int x = (_size.width-w)/2;
        _vboxProgress = new VBox(x, 0, w, h);
        _vboxProgress->setPadding(5);
        _vboxProgress->setSpacing(5);
        _vboxProgress->setBackground({64,64,64});
        _lblProgressTitle = new Label(0,0,0,0, _font, text);
        _lblProgressTitle->setSize(_lblProgressTitle->getAutoSize());
        _lblProgressTitle->setTextAlignment(Label::HAlign::CENTER, Label::VAlign::TOP);
        _vboxProgress->addChild(_lblProgressTitle);
        _pgbProgress = new ProgressBar(1, 0, w-2, 6, max, progress);
        _vboxProgress->addChild(_pgbProgress);
        _hboxProgressTexts = new HBox(0,0,w,0);
        _vboxProgress->addChild(_hboxProgressTexts);
        _lblProgressPercent = new Label(0,0,w/2-5,0, _font, percentText);
        _lblProgressPercent->setTextAlignment(Label::HAlign::LEFT, Label::VAlign::TOP);
        _lblProgressPercent->setHeight(_lblProgressPercent->getAutoHeight());
        _hboxProgressTexts->addChild(_lblProgressPercent);
        _lblProgressValues = new Label(0,0,w/2-5,0, _font, valuesText);
        _lblProgressValues->setTextAlignment(Label::HAlign::RIGHT, Label::VAlign::TOP);
        _lblProgressValues->setHeight(_lblProgressValues->getAutoHeight());
        _hboxProgressTexts->setHeight(_lblProgressValues->getHeight());
        _hboxProgressTexts->addChild(_lblProgressValues);
        addChild(_vboxProgress);
        _vboxProgress->setHeight(h);
        _vboxProgress->setTop((_size.height-_vboxProgress->getHeight())/2);
    } else {
        _lblProgressTitle->setText(text);
        _pgbProgress->setProgress(progress);
        _pgbProgress->setMax(max);
        _lblProgressPercent->setText(percentText);
        _lblProgressValues->setText(valuesText);
        _vboxProgress->setVisible(true);
        raiseChild(_vboxProgress);
    }
}

void DefaultTrackerWindow::hideProgress()
{
    _vboxProgress->setVisible(false);
}

void DefaultTrackerWindow::setAlwaysOnTop(bool alwaysOnTop)
{
    // Color / greyscale represents "always on top" enabled / disabled
    if (_btnAlwaysOnTop) {
        _btnAlwaysOnTop->setEnabled(alwaysOnTop);
    }
    TrackerWindow::setAlwaysOnTop(alwaysOnTop);
}

} // namespace


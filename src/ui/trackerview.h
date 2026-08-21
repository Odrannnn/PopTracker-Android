#ifndef _UI_TRACKERVIEW_H
#define _UI_TRACKERVIEW_H

#include "../uilib/simplecontainer.h"
#include "../uilib/timer.h"
#include "../uilib/group.h"
#include "../uilib/tabs.h"
#include "../uilib/fontstore.h"
#include "../uilib/viewporttransform.h"
#include "mapwidget.h"
#include "maptooltip.h"
#include "item.h"
#include "../core/tracker.h"
#include <list>
#include <map>
#include <vector>

namespace Ui {

class TrackerView : public SimpleContainer {
public:
    struct MobileSection {
        std::string id;
        std::string title;
        std::vector<size_t> path;
        bool fillUndersized = false;
    };

    using FONT = Group::FONT;
    TrackerView(int x, int y, int w, int h, Tracker* tracker, const std::string& layoutRoot, FontStore *fontStore);
    ~TrackerView();

    virtual void render(Renderer renderer, int offX, int offY) override;
    virtual void setSize(Size size) override;
    virtual void addChild(Widget* child) override;
    virtual bool isHit(int x, int y) const override { return Widget::isHit(x, y); }
    void relayout();

    std::list< std::pair<std::string,std::string> > getHints() const;
    const std::string& getLayoutRoot() const;
    Tracker* getTracker();

    void setHideClearedLocations(bool hide);
    void setHideUnreachableLocations(bool hide);
    static std::vector<MobileSection> discoverMobileSections(
        Tracker* tracker, const std::string& layoutRoot
    );
    void setMobileSection(const MobileSection* section);
    float getWorkspaceZoom() const { return _workspaceViewport.getZoom(); }
    void cycleWorkspaceZoom();
    void resetWorkspaceView();

    Signal<const std::string&> onItemHover;
    Signal<const std::string&> onItemTooltip;
    Signal<float> onWorkspaceZoomChanged;

    static int CalculateLocationState(Tracker* tracker, const std::string& location);
    static int CalculateLocationState(Tracker* tracker, const std::string& location,
            const Location::MapLocation& mapLoc);

    static Highlight CalculateLocationHighlight(Tracker* tracker, const std::string& location);
    static Highlight CalculateLocationHighlight(Tracker* tracker, const std::string& location,
            const Location::MapLocation& mapLoc);

protected:
    Tracker* _tracker;
    std::string _layoutRoot;
    std::list<std::string> _layoutRefs;
    bool _relayoutRequired = false;
    FontStore *_fontStore = nullptr;
    FONT _font = nullptr;
    FONT _smallFont = nullptr;
    MapTooltip *_mapTooltip = nullptr;
    Position _mapTooltipPos;
    MapWidget *_mapTooltipOwner = nullptr;
    std::string _mapTooltipName;
    std::map<std::string, int> _mapTooltipScrollOffsets;
    std::string _tooltipItem;
    tick_t _tooltipTimer = 0;
    bool _tooltipTriggered = false;

    int _absX=0;
    int _absY=0;
    std::map<std::string, std::list<Item*>> _items;
    std::map<std::string, std::list<MapWidget*>> _maps;
    bool _mapsDirty = false;
    bool _mapTooltipDirty = false;
    std::list<Tabs*> _tabs;
    std::list<std::string> _activeTabs;
    std::list< std::pair<std::string,std::string> > _missedHints;

    bool _hideClearedLocations = false;
    bool _hideUnreachableLocations = false;

    int _defaultItemQuality = -1;
    int _defaultMapQuality = 2; // default smooth

    /// Android workspace magnification. Unlike MapWidget zoom, this scales
    /// every pack control, including item grids and map selector tabs.
    ViewportTransform _workspaceViewport;
    bool _workspaceDragCandidate = false;
    bool _workspaceDragging = false;
    int _workspaceDragStartX = 0;
    int _workspaceDragStartY = 0;
    float _workspaceDragStartPanX = 0.0f;
    float _workspaceDragStartPanY = 0.0f;
    SDL_Texture *_workspaceTexture = nullptr;
    SDL_Renderer *_workspaceTextureRenderer = nullptr;
    Size _workspaceTextureSize;
    std::string _workspaceStateKey;
    bool _workspaceStateDirty = false;
    bool _suspendWorkspacePersistence = false;
    tick_t _workspaceStateChangedAt = 0;
    std::string _mobileSectionId;
    std::vector<size_t> _mobileSectionPath;
    Size _mobileSectionContentSize;
    bool _mobileSectionFillUndersized = false;
    bool _mobileSectionAutoFitPending = false;
    bool _workspaceStateRestored = false;

    bool prepareMouseDown(int& x, int& y, int button) override;
    bool prepareClick(int& x, int& y, int button) override;
    bool prepareMouseMove(int& x, int& y, unsigned buttons) override;
    bool preparePinch(int& x, int& y, float scale) override;
    void transformWorkspacePoint(int& x, int& y) const;
    void releaseWorkspaceTexture();
    void updateWorkspaceStateKey();
    void markWorkspaceStateChanged();
    void saveWorkspaceState();
    void updateMinimumTouchTargets();
    void workspaceZoomChanged();
    float getMobileSectionFillZoom() const;
    void applyMobileSectionAutoFit();
    bool canPanWorkspace() const;
    void clampWorkspacePanToContent();
    const LayoutNode* resolveMobileSectionNode() const;

    void updateLayout(const std::string& layout);
    void updateDisplay(const std::string& check);
    void updateState(const std::string& check);
    void updateLocations();
    void updateLocationsNow();
    void updateLocation(const std::string& location);
    void updateLocationNow(const std::string& location);
    void updateMapTooltip();
    void updateMapTooltipNow();
    void updateItem(Item* w, const BaseItem& item);

    size_t addLayoutNodes(Container* container, const std::list<LayoutNode>& nodes, size_t depth=0);
    bool addLayoutNode(Container* container, const LayoutNode& node, size_t depth=0);

    Item* makeItem(int x, int y, int w, int h, const std::string& code);
    Item* makeItem(int x, int y, int w, int h, const ::BaseItem& item, int stage1=-1, int stage2=0);
    Item* makeLocationIcon(int x, int y, int w, int h, const std::string& locid, const LocationSection& sec, bool opened, bool compact);

    ScrollVBox* makeMapTooltip(const std::string& location, int x, int y);
};

} // namespace Ui

#endif // _UI_TRACKERVIEW_H

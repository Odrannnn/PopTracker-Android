#pragma once

#include <algorithm>
#include <deque>
#include "widget.h"

#ifndef NDEBUG
#include <typeinfo>
#include <stdio.h>
#endif

namespace Ui {

class Container : public Widget {
public:
    ~Container() override {
        onMouseCancel -= this;
        onMouseDown -= this;
        Container::clearChildren();
    }

    virtual void addChild(Widget* child) {
        if (!child) return;
        _children.push_back(child);
        if (child->getHGrow()>_hGrow) _hGrow = child->getHGrow();
        if (child->getVGrow()>_vGrow) _vGrow = child->getVGrow();
    }

    virtual void removeChild(Widget* child) {
        if (!child) return;
        if (child == _hoverChild) {
            _hoverChild = nullptr;
            child->onMouseLeave.emit(child);
        }
        if (child == _pressedChild) {
            _pressedChild = nullptr;
            child->onMouseCancel.emit(child);
        }
        _children.erase(std::remove(_children.begin(), _children.end(), child), _children.end());
        if ((_hGrow>0 && child->getHGrow()>=_hGrow) || (_vGrow>0 && child->getVGrow()>=_vGrow)) {
            int oldHGrow = _hGrow; int oldVGrow = _vGrow;
            _hGrow = 0; _vGrow = 0;
            for (auto& tmp : _children) {
                if (tmp->getHGrow()>=_hGrow) _hGrow = tmp->getHGrow();
                if (tmp->getVGrow()>=_vGrow) _vGrow = tmp->getVGrow();
                if (_hGrow == oldHGrow && _vGrow == oldVGrow) break;
            }
        }
    }

    virtual bool hasChild(Widget* child)
    {
        return std::find(_children.begin(), _children.end(), child) != _children.end();
    }

    virtual void clearChildren() {
        if (_hoverChild) {
            _hoverChild->onMouseLeave.emit(_hoverChild);
            _hoverChild = nullptr;
        }
        if (_pressedChild) {
            _pressedChild->onMouseCancel.emit(_pressedChild);
            _pressedChild = nullptr;
        }
        for (const auto child : _children) {
            delete child;
        }
        _children.clear();
    }

    virtual void raiseChild(Widget* child) {
        if (!child) return;
        for (auto it=_children.begin(); it!=_children.end(); it++) {
            if (*it == child) {
                _children.erase(it);
                _children.push_back(child);
                return;
            }
        }
    }

    void render(Renderer renderer, const int offX, const int offY) override {
        if (_backgroundColor.a > 0) {
            const auto& c = _backgroundColor;
            SDL_SetRenderDrawColor(renderer, c.r, c.g, c.b, c.a);
            SDL_Rect r = { offX+_pos.left-_margin.left,
                           offY+_pos.top-_margin.top,
                           _size.width+_margin.left+_margin.right,
                           _size.height+_margin.top+_margin.bottom
            };
            SDL_RenderFillRect(renderer, &r);
        }
        // TODO: background image
        for (auto& child: _children)
            if (child->getVisible())
                child->render(renderer, offX+_pos.left, offY+_pos.top);
    }

    const std::deque<Widget*> getChildren() const { return _children; }

    bool isHover(Widget* w) const override {
        return (w == this || (_hoverChild && _hoverChild->isHover(w)));
    }

    bool isHit(int x, int y) const override {
        if (!_mouseInteraction)
            return false;
        if (_backgroundColor.a && Widget::isHit(x, y))
            return true;
        x -= _pos.left;
        y -= _pos.top;
        for (const auto& w: _children) {
            if (w->isHit(x, y))
                return true;
        }
        return false;
    }

    const Widget* getHit(int x, int y) const override {
        for (const auto& w: _children) {
            if (w->getHit(x - w->getLeft(), y - w->getTop())) return w;
        }
        if (Widget::isHit(x + _pos.left, y + _pos.top)) return this;
        return nullptr;
    }

#ifndef NDEBUG
    void dumpTree(size_t depth=0) {
        printf("%s", std::string(depth, ' ').c_str());
        dumpInfo();
        depth += 2;
        for (auto child: _children) {
            Container* c = dynamic_cast<Container*>(child);
            if (c) c->dumpTree(depth);
            else {
                printf("%s", std::string(depth, ' ').c_str());
                child->dumpInfo();
            }
        }
    }
#endif

protected:
    std::deque<Widget*> _children;
    Widget* _hoverChild = nullptr;
    Widget* _pressedChild = nullptr;

    // Prefer normal topmost hit-testing. If a touch lands only inside several
    // expanded accessibility targets, choose the nearest visual centre so
    // dense desktop item grids remain predictable on a phone.
    Widget* findPointerChild(int x, int y) const
    {
        Widget* nearest = nullptr;
        long long nearestDistance = 0;
        for (auto childIt = _children.rbegin(); childIt != _children.rend(); ++childIt) {
            Widget* child = *childIt;
            if (!child->getVisible() || !child->isHit(x, y))
                continue;
            if (child->isVisualHit(x, y) || !child->hasExpandedHitArea())
                return child;
            const long long distance = child->hitCenterDistanceSquared(x, y);
            if (!nearest || distance < nearestDistance) {
                nearest = child;
                nearestDistance = distance;
            }
        }
        return nearest;
    }

    // Specialized containers can map pointer coordinates into a transformed
    // child coordinate space, or consume an interaction before it reaches a
    // child. The default container remains a transparent event router.
    virtual bool prepareMouseDown(int&, int&, int) { return true; }
    virtual bool prepareClick(int&, int&, int) { return true; }
    virtual bool prepareMouseMove(int&, int&, unsigned) { return true; }
    virtual bool preparePinch(int&, int&, float) { return true; }

    explicit Container(const int x=0, const int y=0, const int w=0, const int h=0)
        : Widget(x,y,w,h)
    {
        onMouseDown += { this, [this](void*, int x, int y, const int button) {
            if (!prepareMouseDown(x, y, button))
                return;
            if (auto child = findPointerChild(x, y)) {
                _pressedChild = child;
                child->onMouseDown.emit(child, x - child->getLeft(), y - child->getTop(), button);
            }
        }};
        onClick += { this, [this](void*, int x, int y, const int button) {
            if (!prepareClick(x, y, button))
                return;
            if (auto child = findPointerChild(x, y)) {
                const auto oldPressedChild = _pressedChild;
                if (oldPressedChild == child) {
                    _pressedChild = nullptr;
                    asm volatile("" ::: "memory"); // guarantee the onClick is gonna happen last
                    child->onClick.emit(child, x - child->getLeft(), y - child->getTop(), button);
                    return; // the onClick handler can have side effects, so we need to return immediately after
                }
            }
            if (const auto oldPressedChild = _pressedChild) {
                _pressedChild = nullptr;
                oldPressedChild->onMouseCancel.emit(oldPressedChild);
            }
        }};
        onMouseMove += { this, [this](void*, int x, int y, const unsigned buttons) {
            if (!prepareMouseMove(x, y, buttons))
                return;
            auto oldHoverChild = _hoverChild;
            bool match = false;
            Widget* child = findPointerChild(x, y);
            if (child) {
                if (child != oldHoverChild) {
                    if (oldHoverChild) {
                        _hoverChild = nullptr;
                        oldHoverChild->onMouseLeave.emit(oldHoverChild);
                    }
                    if (!hasChild(child)) // the above event could modify the container
                        child = nullptr;
                    _hoverChild = child;
                    if (child)
                        child->onMouseEnter.emit(_hoverChild, x-child->getLeft(), y-child->getTop(), buttons);
                }
                if (child) {
                    child->onMouseMove.emit(child, x-child->getLeft(), y-child->getTop(), buttons);
                    match = true;
                }
            }
            if (!match)
                _hoverChild = nullptr;
            if (oldHoverChild && !_hoverChild)
                oldHoverChild->onMouseLeave.emit(oldHoverChild);
        }};
        onMouseLeave += { this, [this](void*) {
            if (const auto oldHoverChild = _hoverChild) {
                _hoverChild = nullptr;
                oldHoverChild->onMouseLeave.emit(oldHoverChild);
            }
        }};
        onMouseCancel += { this, [this](void*) {
            if (const auto oldPressedChild = _pressedChild) {
                _pressedChild = nullptr;
                oldPressedChild->onMouseCancel.emit(oldPressedChild);
            }
        }};
        onScroll += { this, [this](void*, const int x, const int y, const unsigned mod) {
            if (_hoverChild)
                _hoverChild->onScroll.emit(_hoverChild, x, y, mod);
        }};
        onPinch += { this, [this](void*, int x, int y, const float scale) {
            if (!preparePinch(x, y, scale))
                return;
            if (auto child = findPointerChild(x, y)) {
                child->onPinch.emit(child,
                    x - child->getLeft(), y - child->getTop(), scale);
            }
        }};
    }
};

} // namespace Ui

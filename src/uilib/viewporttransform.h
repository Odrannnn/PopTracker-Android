#pragma once

#include "size.h"
#include <SDL2/SDL_rect.h>
#include <algorithm>
#include <cmath>

namespace Ui {

// Maps an unchanged pack-authored logical canvas into a mobile viewport.
// Pack widgets always render and receive events in logical coordinates; only
// this transform knows about magnification and panning.
class ViewportTransform {
public:
    struct State {
        float zoom = 1.0f;
        float panX = 0.0f; // normalized against the current maximum pan
        float panY = 0.0f;
    };

    explicit ViewportTransform(Size viewport = {}) : _viewport(viewport) {}

    void setViewport(Size viewport)
    {
        const State state = getState();
        _viewport = viewport;
        restore(state);
    }

    const Size& getViewport() const { return _viewport; }
    float getZoom() const { return _zoom; }
    float getPanX() const { return _panX; }
    float getPanY() const { return _panY; }
    bool isMagnified() const { return _zoom > 1.001f; }

    bool reset()
    {
        if (!isMagnified() && _panX == 0.0f && _panY == 0.0f)
            return false;
        _zoom = 1.0f;
        _panX = 0.0f;
        _panY = 0.0f;
        return true;
    }

    bool setZoomAround(float zoom, int screenX, int screenY)
    {
        if (!std::isfinite(zoom))
            return false;
        zoom = std::max(MIN_ZOOM, std::min(MAX_ZOOM, zoom));
        if (zoom < 1.01f)
            zoom = 1.0f;
        if (std::abs(zoom - _zoom) < 0.0001f)
            return false;

        const float logicalX = _panX + screenX / _zoom;
        const float logicalY = _panY + screenY / _zoom;
        _zoom = zoom;
        _panX = logicalX - screenX / _zoom;
        _panY = logicalY - screenY / _zoom;
        clamp();
        return true;
    }

    void setPan(float panX, float panY)
    {
        _panX = panX;
        _panY = panY;
        clamp();
    }

    void screenToLogical(int& x, int& y) const
    {
        x = static_cast<int>(_panX + x / _zoom);
        y = static_cast<int>(_panY + y / _zoom);
    }

    SDL_Rect getSourceRect() const
    {
        const int sourceWidth = std::max(1,
            static_cast<int>(_viewport.width / _zoom + 0.5f));
        const int sourceHeight = std::max(1,
            static_cast<int>(_viewport.height / _zoom + 0.5f));
        return {
            static_cast<int>(_panX + 0.5f),
            static_cast<int>(_panY + 0.5f),
            std::min(sourceWidth, _viewport.width),
            std::min(sourceHeight, _viewport.height)
        };
    }

    State getState() const
    {
        return {
            _zoom,
            maxPanX() > 0.0f ? _panX / maxPanX() : 0.0f,
            maxPanY() > 0.0f ? _panY / maxPanY() : 0.0f
        };
    }

    bool restore(State state)
    {
        if (!std::isfinite(state.zoom) || !std::isfinite(state.panX) ||
                !std::isfinite(state.panY))
            return false;
        _zoom = std::max(MIN_ZOOM, std::min(MAX_ZOOM, state.zoom));
        if (_zoom < 1.01f)
            _zoom = 1.0f;
        state.panX = std::max(0.0f, std::min(1.0f, state.panX));
        state.panY = std::max(0.0f, std::min(1.0f, state.panY));
        _panX = state.panX * maxPanX();
        _panY = state.panY * maxPanY();
        clamp();
        return true;
    }

private:
    static constexpr float MIN_ZOOM = 1.0f;
    static constexpr float MAX_ZOOM = 6.0f;

    float maxPanX() const
    {
        return isMagnified()
            ? std::max(0.0f, _viewport.width - _viewport.width / _zoom)
            : 0.0f;
    }

    float maxPanY() const
    {
        return isMagnified()
            ? std::max(0.0f, _viewport.height - _viewport.height / _zoom)
            : 0.0f;
    }

    void clamp()
    {
        if (!isMagnified()) {
            _zoom = 1.0f;
            _panX = 0.0f;
            _panY = 0.0f;
            return;
        }
        _panX = std::max(0.0f, std::min(maxPanX(), _panX));
        _panY = std::max(0.0f, std::min(maxPanY(), _panY));
    }

    Size _viewport;
    float _zoom = 1.0f;
    float _panX = 0.0f;
    float _panY = 0.0f;
};

} // namespace Ui

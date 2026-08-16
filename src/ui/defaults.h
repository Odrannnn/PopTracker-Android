#ifndef _UI_DEFAULTS_H
#define _UI_DEFAULTS_H


#include "../uilib/timer.h"
#include <SDL2/SDL.h>
#include <algorithm>


namespace Ui {

constexpr static char DEFAULT_FONT_NAME[] = "DejaVuSans-Bold.ttf";
constexpr static int DEFAULT_FONT_SIZE = 11; // actually ~10px
constexpr static tick_t DEFAULT_TOOLTIP_DELAY = 1000; // 1 sec

struct NativeUiMetrics {
    int toolbarHeight = 32;
    int toolbarPadding = 2;
    int toolbarSpacing = 2;
    int touchTarget = 28;
    int toolbarIconSize = 28;
    int toolbarCornerRadius = 0;
    int tooltipHeight = 28;
    int tooltipFontSize = DEFAULT_FONT_SIZE;
    int overlayMargin = 2;
    int fontSize = DEFAULT_FONT_SIZE;
    int packRowHeight = 32;
    int panelPadding = 2;
    int panelSpacing = 1;
};

// Tracker packs describe their own pixel-perfect layouts, but PopTracker's
// surrounding controls need physical, finger-friendly dimensions on Android.
// Phones use the standard 56dp app bar and tablets use a slightly denser 52dp
// bar; both retain a minimum 48dp touch target.
inline NativeUiMetrics getNativeUiMetrics()
{
    NativeUiMetrics metrics;
#ifdef __ANDROID__
    float densityDpi = 160.0f;
    if (SDL_GetDisplayDPI(0, &densityDpi, nullptr, nullptr) != 0 || densityDpi <= 0)
        densityDpi = 160.0f;
    const float density = std::max(1.0f, std::min(4.0f, densityDpi / 160.0f));

    SDL_Rect bounds{0, 0, 0, 0};
    const bool haveBounds = SDL_GetDisplayBounds(0, &bounds) == 0;
    const float shortestWidthDp = haveBounds
        ? std::min(bounds.w, bounds.h) / density
        : 400.0f;
    const bool phone = shortestWidthDp < 600.0f;
    const auto px = [density](float dp) {
        return std::max(1, static_cast<int>(dp * density + 0.5f));
    };

    metrics.toolbarHeight = px(phone ? 56.0f : 52.0f);
    metrics.toolbarPadding = px(phone ? 4.0f : 2.0f);
    metrics.toolbarSpacing = px(4.0f);
    metrics.touchTarget = px(48.0f);
    metrics.toolbarIconSize = px(phone ? 28.0f : 26.0f);
    metrics.toolbarCornerRadius = px(12.0f);
    metrics.tooltipHeight = px(48.0f);
    metrics.tooltipFontSize = px(phone ? 14.0f : 13.0f);
    metrics.overlayMargin = px(8.0f);
    metrics.fontSize = px(phone ? 18.0f : 16.0f);
    metrics.packRowHeight = px(phone ? 56.0f : 52.0f);
    metrics.panelPadding = px(4.0f);
    metrics.panelSpacing = px(1.0f);
#endif
    return metrics;
}

} // namespace Ui

#endif // _UI_DEFAULTS_H

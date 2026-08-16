#include "imagebutton.h"
#include "../core/fs.h"
#include "drawhelper.h"
#include <algorithm>

namespace Ui {

ImageButton::ImageButton(int x, int y, int w, int h, const fs::path& path)
    : Image(x,y,w,h,path)
{
    setCursor(Cursor::HAND);
    onMouseDown += {this, [this](void*, int, int, int button) {
        if (button == MouseButton::BUTTON_LEFT)
            _pressed = true;
    }};
    onClick += {this, [this](void*, int, int, int) { _pressed = false; }};
    onMouseCancel += {this, [this](void*) { _pressed = false; }};
}

ImageButton::ImageButton(int x, int y, int w, int h, const void* data, size_t len)
    : Image(x,y,w,h,data,len)
{
    setCursor(Cursor::HAND);
    onMouseDown += {this, [this](void*, int, int, int button) {
        if (button == MouseButton::BUTTON_LEFT)
            _pressed = true;
    }};
    onClick += {this, [this](void*, int, int, int) { _pressed = false; }};
    onMouseCancel += {this, [this](void*) { _pressed = false; }};
}

void ImageButton::render(Renderer renderer, int offX, int offY)
{
    if (_backgroundColor.a > 0) {
        auto color = _backgroundColor;
        if (_pressed) {
            color.r = static_cast<uint8_t>(std::min(255, color.r + 24));
            color.g = static_cast<uint8_t>(std::min(255, color.g + 24));
            color.b = static_cast<uint8_t>(std::min(255, color.b + 24));
            color.a = static_cast<uint8_t>(std::min(255, color.a + 28));
        }
        const SDL_Rect rect = {offX + _pos.left, offY + _pos.top, _size.width, _size.height};
        fillRoundedRect(renderer, rect, _cornerRadius, color);
    }

    const Position oldPosition = _pos;
    const Size oldSize = _size;
    const Color oldBackground = _backgroundColor;
    _pos = {_pos.left + _contentPadding, _pos.top + _contentPadding};
    _size = {
        std::max(1, _size.width - 2 * _contentPadding),
        std::max(1, _size.height - 2 * _contentPadding)
    };
    _backgroundColor.a = 0;
    Image::render(renderer, offX, offY);
    _backgroundColor = oldBackground;
    _size = oldSize;
    _pos = oldPosition;
}

} // namespace


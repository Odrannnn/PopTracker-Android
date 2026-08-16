#ifndef _UILIB_IMAGEBUTTON_H
#define _UILIB_IMAGEBUTTON_H

#include "image.h"
#include "../core/fs.h"

namespace Ui {

class ImageButton : public Image {
public:
    ImageButton(int x, int y, int w, int h, const fs::path& path);
    ImageButton(int x, int y, int w, int h, const void* data, size_t len);
    void render(Renderer renderer, int offX, int offY) override;
    void setContentPadding(int padding) { _contentPadding = padding > 0 ? padding : 0; }

protected:
    int _contentPadding = 0;
    bool _pressed = false;
};

} // namespace Ui

#endif // _UILIB_IMAGEBUTTON_H

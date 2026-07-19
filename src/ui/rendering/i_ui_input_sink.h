#pragma once

namespace kfc {

class IUiInputSink {
public:
    virtual ~IUiInputSink();
    virtual void on_pixel_click(int x, int y) = 0;
    virtual void on_pixel_jump(int x, int y) = 0;
};

}  // namespace kfc

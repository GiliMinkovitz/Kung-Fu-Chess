// src/ui/ui_controller.h
#include "i_ui_renderer.h"
#include <memory>

namespace kfc {
    class UIController {
    private:
        std::unique_ptr<IUIRenderer> renderer_;

    public:
        explicit UIController(std::unique_ptr<IUIRenderer> renderer) 
            : renderer_(std::move(renderer)) {}

        void start() {
            renderer_->initialize();
        }
    };
}
#pragma once

#include "board_layout_calculator.h"
#include "i_ui_renderer.h"

namespace kfc {

class HeadlessRenderer final : public IUiRenderer {
public:
    void init(int window_width, int window_height, std::size_t rows, std::size_t cols) override;
    void attach_input_sink(IUiInputSink* sink) override;
    [[nodiscard]] BoardLayout board_layout() const override;
    UiFrameResult present(const BoardViewModel& view) override;
    void shutdown() override;

    [[nodiscard]] bool initialized() const noexcept { return initialized_; }
    [[nodiscard]] std::size_t rows() const noexcept { return rows_; }
    [[nodiscard]] std::size_t cols() const noexcept { return cols_; }
    [[nodiscard]] int window_width() const noexcept { return window_width_; }
    [[nodiscard]] int window_height() const noexcept { return window_height_; }
    [[nodiscard]] IUiInputSink* input_sink() const noexcept { return input_sink_; }
    [[nodiscard]] const BoardViewModel& last_view() const noexcept { return last_view_; }
    [[nodiscard]] std::size_t present_count() const noexcept { return present_count_; }
    [[nodiscard]] bool shutdown_called() const noexcept { return shutdown_called_; }

    void set_should_continue(bool value) { should_continue_ = value; }

private:
    BoardLayoutCalculator layout_calculator_;
    BoardLayout layout_{};
    std::size_t rows_ = 0;
    std::size_t cols_ = 0;
    int window_width_ = 0;
    int window_height_ = 0;
    bool initialized_ = false;
    bool shutdown_called_ = false;
    bool should_continue_ = true;
    std::size_t present_count_ = 0;
    IUiInputSink* input_sink_ = nullptr;
    BoardViewModel last_view_;
};

}  // namespace kfc

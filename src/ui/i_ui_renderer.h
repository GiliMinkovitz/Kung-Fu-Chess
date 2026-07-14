namespace kfc {
    class IUIRenderer {
    public:
        virtual ~IUIRenderer() = default;
        virtual void initialize() = 0;
        virtual void render_board() = 0;
        virtual void poll_events() = 0; // טיפול ב-Mouse Events
    };
}
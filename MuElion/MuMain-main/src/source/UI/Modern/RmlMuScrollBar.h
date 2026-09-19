#pragma once

#include <RmlUi/Core/EventListener.h>

#include <cstddef>
#include <memory>
#include <optional>

namespace Rml
{
class Element;
class Event;
}

namespace UI::Modern
{

// Vertical scrollbar reconstructed from Main-x64-Debug.exe RTTI and direct
// x64 function-body analysis. Event names, child ordering, range math and drag
// behavior are recovered; public non-virtual names are semantic reconstruction
// names because private identifiers are not present in the executable.
class RmlMuScrollBar
{
public:
    RmlMuScrollBar();
    ~RmlMuScrollBar();

    RmlMuScrollBar(const RmlMuScrollBar&) = delete;
    RmlMuScrollBar& operator=(const RmlMuScrollBar&) = delete;

    // The recovered control expects exactly four DOM children:
    //   0 = track, 1 = increment/down, 2 = decrement/up, 3 = thumb.
    [[nodiscard]] bool Bind(Rml::Element* root);
    void Unbind();

    // maximum is the largest scroll position. visibleAmount is the number of
    // visible logical units used for thumb sizing. pageStep==0 falls back to
    // max(1, visibleAmount), matching the recovered track-click behavior.
    [[nodiscard]] bool SetState(std::size_t position,
                                std::size_t maximum,
                                std::size_t visibleAmount,
                                std::size_t pageStep,
                                bool visible = true,
                                bool enabled = true);

    [[nodiscard]] std::size_t GetPosition() const noexcept;
    [[nodiscard]] std::size_t GetMaximum() const noexcept;
    [[nodiscard]] bool IsDragging() const noexcept;

    // Returns a requested position once. Event handlers do not mutate the
    // externally-owned position immediately; the Debug control stores a
    // std::optional<size_t> request which the owner consumes.
    [[nodiscard]] std::optional<std::size_t> ConsumeRequestedPosition();

    // Re-reads element boxes and synchronizes thumb top/height, display and
    // enabled classes. Returns true when a visible property changed.
    [[nodiscard]] bool Update();

private:
    class Listener final : public Rml::EventListener
    {
    public:
        explicit Listener(RmlMuScrollBar* owner) noexcept : owner_(owner) {}
        ~Listener() override = default;
        void ProcessEvent(Rml::Event& event) override;

    private:
        RmlMuScrollBar* owner_ = nullptr;
    };

    void OnEvent(Rml::Event& event);
    void RequestPosition(std::size_t position) noexcept;
    [[nodiscard]] float TrackTop() const;
    [[nodiscard]] float TrackHeight() const;
    [[nodiscard]] float ThumbTop() const;
    [[nodiscard]] float ThumbHeight() const;
    [[nodiscard]] float ThumbTravel() const;
    [[nodiscard]] float PositionToOffset(std::size_t position) const;
    [[nodiscard]] std::size_t DragDeltaToPosition(float deltaY) const;
    [[nodiscard]] static bool SetPixelProperty(
        Rml::Element* element, const char* name, float value);

    // Recovered x64 layout begins with:
    // +0x00 unique_ptr<Listener>
    // +0x08 root
    // +0x10 track
    // +0x18 increment/down
    // +0x20 decrement/up
    // +0x28 thumb
    // +0x30 position
    // +0x38 maximum
    // +0x40 visible/page fallback amount
    // +0x50 explicit page step
    //
    // The original then stores cached layout/style state, a
    // std::optional<size_t> request near +0xa0, drag scale at +0xb0,
    // drag-start mouse Y at +0xb4, drag-start position at +0xb8 and the
    // dragging byte at +0xc0.
    std::unique_ptr<Listener> listener_;
    Rml::Element* root_ = nullptr;
    Rml::Element* track_ = nullptr;
    Rml::Element* increment_ = nullptr;
    Rml::Element* decrement_ = nullptr;
    Rml::Element* thumb_ = nullptr;

    std::size_t position_ = 0;
    std::size_t maximum_ = 0;
    std::size_t visibleAmount_ = 0;
    std::size_t pageStep_ = 0;
    bool visible_ = true;
    bool enabled_ = true;

    std::optional<std::size_t> requestedPosition_;

    float dragPixelsPerPosition_ = 1.0f;
    float dragStartMouseY_ = 0.0f;
    std::size_t dragStartPosition_ = 0;
    bool dragging_ = false;

    std::optional<float> appliedThumbTop_;
    std::optional<float> appliedThumbHeight_;
    std::optional<bool> appliedVisible_;
    std::optional<bool> appliedIncrementEnabled_;
    std::optional<bool> appliedDecrementEnabled_;
};

} // namespace UI::Modern

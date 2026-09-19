#pragma once

#include "UI/Modern/RmlMuButton.h"

#include <RmlUi/Core/EventListener.h>

#include <atomic>
#include <memory>

namespace Rml
{
class Element;
class Event;
}

namespace UI::Modern
{

// Slot control reconstructed from Main-x64-Debug.exe. The class derives from
// RmlMuButton in the recovered layout, adds a second mouse-button latch and an
// icon-frame CSS class selector.
class RmlMuSlot final : public RmlMuButton
{
public:
    RmlMuSlot();
    ~RmlMuSlot();

    RmlMuSlot(const RmlMuSlot&) = delete;
    RmlMuSlot& operator=(const RmlMuSlot&) = delete;

    void Bind(Rml::Element* element);
    void Unbind();

    // Clears both the inherited primary-click latch and the recovered
    // secondary-button latch.
    void ClearClicks() noexcept;
    [[nodiscard]] bool ConsumeSecondaryClicked() noexcept;

    void SetVisible(bool visible) noexcept;
    [[nodiscard]] bool Update();

    [[nodiscard]] bool Contains(Rml::Element* element) const noexcept;

    // frame == 0 means no icon-frame-N class.
    void SetIconFrame(int frame);
    [[nodiscard]] int GetIconFrame() const noexcept;

private:
    class Listener final : public Rml::EventListener
    {
    public:
        explicit Listener(RmlMuSlot* owner) noexcept : owner_(owner) {}
        ~Listener() override = default;
        void ProcessEvent(Rml::Event& event) override;

    private:
        RmlMuSlot* owner_ = nullptr;
    };

    void OnMouseUp(Rml::Event& event) noexcept;
    [[nodiscard]] static Rml::String IconFrameClass(int frame);

    // x64 layout after the 0x18-byte RmlMuButton base:
    // +0x18 unique_ptr<Listener>
    // +0x20 Element*
    // +0x28 atomic<bool> secondaryClicked
    // +0x2c int iconFrame
    std::unique_ptr<Listener> listener_;
    Rml::Element* slotElement_ = nullptr;
    std::atomic<bool> secondaryClicked_{false};
    int iconFrame_ = 0;
};

} // namespace UI::Modern

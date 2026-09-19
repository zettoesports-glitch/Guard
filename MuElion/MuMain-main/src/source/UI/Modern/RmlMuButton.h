#pragma once

#include <RmlUi/Core/EventListener.h>

#include <atomic>
#include <memory>
#include <optional>

namespace Rml
{
class Element;
class Event;
}

namespace UI::Modern
{

// Reconstructed from Main-x64-Debug.exe around the RTTI-exposed nested
// RmlMuButton::Listener. Non-virtual method names are reconstruction-local;
// the observable state transitions and RmlUi event contract are Debug-derived.
class RmlMuButton
{
public:
    RmlMuButton();
    ~RmlMuButton();

    RmlMuButton(const RmlMuButton&) = delete;
    RmlMuButton& operator=(const RmlMuButton&) = delete;

    void Bind(Rml::Element* element);
    void Unbind();

    void SetEnabled(bool enabled) noexcept;
    void SetVisible(bool visible) noexcept;

    [[nodiscard]] bool IsEnabled() const noexcept;
    [[nodiscard]] bool IsVisible() const noexcept;

    void ClearClicked() noexcept;
    [[nodiscard]] bool ConsumeClicked() noexcept;

    // True only while enabled+visible and when element is the bound element or
    // one of its descendants.
    [[nodiscard]] bool Contains(Rml::Element* element) const noexcept;

    // Synchronizes the recovered CSS-facing state:
    //   visible=false -> class "mu-hidden"
    //   enabled=false -> class "disabled", pointer-events:none
    // Returns true if an observable state changed.
    [[nodiscard]] bool Update();

private:
    class Listener final : public Rml::EventListener
    {
    public:
        explicit Listener(RmlMuButton* owner) noexcept : owner_(owner) {}
        ~Listener() override = default;

        void ProcessEvent(Rml::Event& event) override;

    private:
        RmlMuButton* owner_ = nullptr;
    };

    void OnClick() noexcept;

    // x64 Debug layout evidence:
    // +0x00 unique_ptr<Listener>
    // +0x08 Element*
    // +0x10/+0x11/+0x12 atomic<bool> enabled/visible/clicked
    // +0x13/+0x15 optional<bool> applied enabled/visible state
    std::unique_ptr<Listener> listener_;
    Rml::Element* element_ = nullptr;
    std::atomic<bool> enabled_{true};
    std::atomic<bool> visible_{true};
    std::atomic<bool> clicked_{false};
    std::optional<bool> appliedEnabled_;
    std::optional<bool> appliedVisible_;
};

} // namespace UI::Modern

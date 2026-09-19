#pragma once

#include "UI/Modern/RmlMuButton.h"

#include <RmlUi/Core/EventListener.h>

#include <memory>
#include <optional>

namespace Rml
{
class Element;
class Event;
}

namespace UI::Modern
{

// Reconstructed from Main-x64-Debug.exe RTTI for
// RmlMuOptionStepper::Listener and direct function-body analysis.
// Non-virtual method names are semantic reconstruction names; the recovered
// layout, key handling, state transitions and one-shot pending value are kept.
class RmlMuOptionStepper
{
public:
    RmlMuOptionStepper();
    ~RmlMuOptionStepper();

    RmlMuOptionStepper(const RmlMuOptionStepper&) = delete;
    RmlMuOptionStepper& operator=(const RmlMuOptionStepper&) = delete;

    void Bind(Rml::Element* element,
              Rml::Element* decrementButton,
              Rml::Element* incrementButton);
    void Unbind();

    // Applies the external value/range/enabled state and synchronizes the two
    // embedded buttons. Returns true when state or visible button styling
    // changed.
    [[nodiscard]] bool SetState(int value, int maximum, bool enabled);

    // Merges button clicks and keyboard changes into one pending value, updates
    // the local current value, then consumes that pending value exactly once.
    [[nodiscard]] std::optional<int> ConsumeChangedValue();

private:
    class Listener final : public Rml::EventListener
    {
    public:
        explicit Listener(RmlMuOptionStepper* owner) noexcept : owner_(owner) {}
        ~Listener() override = default;

        void ProcessEvent(Rml::Event& event) override;

    private:
        // Debug listener layout stores the owner pointer at +0x10.
        RmlMuOptionStepper* owner_ = nullptr;
    };

    void OnEvent(Rml::Event& event);

    // x64 Debug layout:
    // +0x00 unique_ptr<Listener>
    // +0x08 Element*
    // +0x10 RmlMuButton decrement (0x18 bytes)
    // +0x28 RmlMuButton increment (0x18 bytes)
    // +0x40 int current value
    // +0x44 int maximum index
    // +0x48 bool enabled
    // +0x4c optional<int> pending changed value
    std::unique_ptr<Listener> listener_;
    Rml::Element* element_ = nullptr;
    RmlMuButton decrementButton_;
    RmlMuButton incrementButton_;
    int value_ = 0;
    int maximum_ = 0;
    bool enabled_ = false;
    std::optional<int> pendingValue_;
};

} // namespace UI::Modern

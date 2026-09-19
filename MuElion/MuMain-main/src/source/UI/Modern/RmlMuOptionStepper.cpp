#include "stdafx.h"
#include "UI/Modern/RmlMuOptionStepper.h"

#include <RmlUi/Core/Element.h>
#include <RmlUi/Core/Event.h>
#include <RmlUi/Core/Input.h>

#include <algorithm>
#include <utility>

namespace UI::Modern
{

RmlMuOptionStepper::RmlMuOptionStepper()
    : listener_(std::make_unique<Listener>(this))
{
}

RmlMuOptionStepper::~RmlMuOptionStepper()
{
    Unbind();
}

void RmlMuOptionStepper::Bind(
    Rml::Element* element,
    Rml::Element* decrementButton,
    Rml::Element* incrementButton)
{
    Unbind();

    element_ = element;
    if (element_)
        element_->AddEventListener("keydown", listener_.get(), false);

    decrementButton_.Bind(decrementButton);
    incrementButton_.Bind(incrementButton);
}

void RmlMuOptionStepper::Unbind()
{
    if (element_)
        element_->RemoveEventListener("keydown", listener_.get(), false);

    element_ = nullptr;
    decrementButton_.Unbind();
    incrementButton_.Unbind();
    pendingValue_.reset();
    maximum_ = 0;
    value_ = 0;
    enabled_ = false;
}

bool RmlMuOptionStepper::SetState(
    int value, int maximum, bool enabled)
{
    const bool stateChanged =
        value_ != value ||
        maximum_ != maximum ||
        enabled_ != enabled;

    value_ = value;
    maximum_ = maximum;
    enabled_ = enabled;

    decrementButton_.SetEnabled(enabled && value > 0);
    incrementButton_.SetEnabled(enabled && value < maximum);

    if (!enabled)
        pendingValue_.reset();

    const bool decrementChanged = decrementButton_.Update();
    const bool incrementChanged = incrementButton_.Update();

    return stateChanged || decrementChanged || incrementChanged;
}

std::optional<int> RmlMuOptionStepper::ConsumeChangedValue()
{
    int candidate = pendingValue_.value_or(value_);

    const bool decrement = decrementButton_.ConsumeClicked();
    const bool increment = incrementButton_.ConsumeClicked();

    if (enabled_ && (decrement || increment))
    {
        candidate += static_cast<int>(increment);
        candidate -= static_cast<int>(decrement);
        pendingValue_ = std::clamp(candidate, 0, maximum_);
    }

    if (pendingValue_)
        value_ = *pendingValue_;

    return std::exchange(pendingValue_, std::nullopt);
}

void RmlMuOptionStepper::OnEvent(Rml::Event& event)
{
    const int keyIdentifier =
        event.GetParameter<int>("key_identifier", 0);
    int candidate = pendingValue_.value_or(value_);

    switch (static_cast<Rml::Input::KeyIdentifier>(keyIdentifier))
    {
    case Rml::Input::KI_TAB:
        event.StopPropagation();
        return;

    case Rml::Input::KI_END:
        candidate = maximum_;
        break;

    case Rml::Input::KI_HOME:
        candidate = 0;
        break;

    case Rml::Input::KI_LEFT:
        --candidate;
        break;

    case Rml::Input::KI_RIGHT:
        ++candidate;
        break;

    default:
        return;
    }

    if (enabled_)
        pendingValue_ = std::clamp(candidate, 0, maximum_);

    event.StopPropagation();
}

void RmlMuOptionStepper::Listener::ProcessEvent(Rml::Event& event)
{
    if (owner_)
        owner_->OnEvent(event);
}

} // namespace UI::Modern

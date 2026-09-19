#include "stdafx.h"
#include "UI/Modern/RmlMuButton.h"

#include <RmlUi/Core/Element.h>
#include <RmlUi/Core/Event.h>

namespace UI::Modern
{

RmlMuButton::RmlMuButton()
    : listener_(std::make_unique<Listener>(this))
{
}

RmlMuButton::~RmlMuButton()
{
    Unbind();
}

void RmlMuButton::Bind(Rml::Element* element)
{
    Unbind();

    element_ = element;
    if (element_)
        element_->AddEventListener("click", listener_.get(), false);

    appliedEnabled_.reset();
    appliedVisible_.reset();
}

void RmlMuButton::Unbind()
{
    if (element_)
        element_->RemoveEventListener("click", listener_.get(), false);

    element_ = nullptr;
    appliedEnabled_.reset();
    appliedVisible_.reset();
    clicked_.store(false, std::memory_order_release);
}

void RmlMuButton::SetEnabled(bool enabled) noexcept
{
    enabled_.store(enabled, std::memory_order_release);
    if (!enabled)
        clicked_.store(false, std::memory_order_release);
}

void RmlMuButton::SetVisible(bool visible) noexcept
{
    visible_.store(visible, std::memory_order_release);
    if (!visible)
        clicked_.store(false, std::memory_order_release);
}

bool RmlMuButton::IsEnabled() const noexcept
{
    return enabled_.load(std::memory_order_acquire);
}

bool RmlMuButton::IsVisible() const noexcept
{
    return visible_.load(std::memory_order_acquire);
}

void RmlMuButton::ClearClicked() noexcept
{
    clicked_.store(false, std::memory_order_release);
}

bool RmlMuButton::ConsumeClicked() noexcept
{
    return clicked_.exchange(false, std::memory_order_acq_rel);
}

bool RmlMuButton::Contains(Rml::Element* element) const noexcept
{
    if (!IsEnabled() || !IsVisible())
        return false;

    for (Rml::Element* current = element; current; current = current->GetParentNode())
    {
        if (current == element_)
            return true;
    }
    return false;
}

bool RmlMuButton::Update()
{
    if (!element_)
        return false;

    bool changed = false;

    const bool visible = IsVisible();
    if (!appliedVisible_.has_value() || *appliedVisible_ != visible)
    {
        element_->SetClass("mu-hidden", !visible);
        appliedVisible_ = visible;
        changed = true;
    }

    const bool enabled = IsEnabled();
    if (!appliedEnabled_.has_value() || *appliedEnabled_ != enabled)
    {
        element_->SetClass("disabled", !enabled);
        element_->SetProperty("pointer-events", enabled ? "auto" : "none");
        appliedEnabled_ = enabled;
        changed = true;
    }

    return changed;
}

void RmlMuButton::OnClick() noexcept
{
    if (IsEnabled() && IsVisible())
        clicked_.store(true, std::memory_order_release);
}

void RmlMuButton::Listener::ProcessEvent(Rml::Event&)
{
    if (owner_)
        owner_->OnClick();
}

} // namespace UI::Modern

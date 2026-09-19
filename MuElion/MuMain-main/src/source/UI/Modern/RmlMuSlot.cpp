#include "stdafx.h"
#include "UI/Modern/RmlMuSlot.h"

#include <RmlUi/Core/Element.h>
#include <RmlUi/Core/Event.h>

#include <string>

namespace UI::Modern
{

RmlMuSlot::RmlMuSlot()
    : listener_(std::make_unique<Listener>(this))
{
}

RmlMuSlot::~RmlMuSlot()
{
    Unbind();
}

void RmlMuSlot::Bind(Rml::Element* element)
{
    Unbind();

    slotElement_ = element;
    RmlMuButton::Bind(element);

    if (slotElement_)
        slotElement_->AddEventListener("mouseup", listener_.get(), false);
}

void RmlMuSlot::Unbind()
{
    if (slotElement_)
    {
        if (iconFrame_ != 0)
            slotElement_->SetClass(IconFrameClass(iconFrame_), false);

        slotElement_->RemoveEventListener("mouseup", listener_.get(), false);
    }

    RmlMuButton::Unbind();
    slotElement_ = nullptr;
    iconFrame_ = 0;
    secondaryClicked_.store(false, std::memory_order_release);
}

void RmlMuSlot::ClearClicks() noexcept
{
    RmlMuButton::ClearClicked();
    secondaryClicked_.store(false, std::memory_order_release);
}

bool RmlMuSlot::ConsumeSecondaryClicked() noexcept
{
    // The Debug build uses atomic exchange(false, acquire/release) here.
    return secondaryClicked_.exchange(false, std::memory_order_acq_rel);
}

void RmlMuSlot::SetVisible(bool visible) noexcept
{
    RmlMuButton::SetVisible(visible);
    if (!visible)
        secondaryClicked_.store(false, std::memory_order_release);
}

bool RmlMuSlot::Update()
{
    return RmlMuButton::Update();
}

bool RmlMuSlot::Contains(Rml::Element* element) const noexcept
{
    return RmlMuButton::Contains(element);
}

Rml::String RmlMuSlot::IconFrameClass(int frame)
{
    return Rml::String("icon-frame-") + std::to_string(frame);
}

void RmlMuSlot::SetIconFrame(int frame)
{
    if (iconFrame_ == frame)
        return;

    if (slotElement_ && iconFrame_ != 0)
        slotElement_->SetClass(IconFrameClass(iconFrame_), false);

    iconFrame_ = frame;

    if (slotElement_ && iconFrame_ != 0)
        slotElement_->SetClass(IconFrameClass(iconFrame_), true);
}

int RmlMuSlot::GetIconFrame() const noexcept
{
    return iconFrame_;
}

void RmlMuSlot::OnMouseUp(Rml::Event& event) noexcept
{
    // Exact recovered parameter name and comparison. In RmlUi's mouse event
    // convention this is the secondary/right mouse button path.
    if (event.GetParameter<int>("button", 0) == 1)
        secondaryClicked_.store(true, std::memory_order_release);
}

void RmlMuSlot::Listener::ProcessEvent(Rml::Event& event)
{
    if (owner_)
        owner_->OnMouseUp(event);
}

} // namespace UI::Modern

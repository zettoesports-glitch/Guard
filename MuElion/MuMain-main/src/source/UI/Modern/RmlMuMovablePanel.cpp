#include "stdafx.h"
#include "UI/Modern/RmlMuMovablePanel.h"

#include <RmlUi/Core/Element.h>
#include <RmlUi/Core/Event.h>

#include <algorithm>
#include <array>
#include <cstdio>

namespace UI::Modern
{

namespace
{
Rml::String PixelValue(float value)
{
    std::array<char, 64> buffer{};
    std::snprintf(buffer.data(), buffer.size(), "%.3fpx",
                  static_cast<double>(value));
    return Rml::String(buffer.data());
}
} // namespace

RmlMuMovablePanel::RmlMuMovablePanel()
    : listener_(std::make_unique<Listener>(this))
{
}

RmlMuMovablePanel::~RmlMuMovablePanel()
{
    Unbind();
}

void RmlMuMovablePanel::Bind(Rml::Element* panel, Rml::Element* dragHandle)
{
    Unbind();

    panel_ = panel;
    dragHandle_ = dragHandle;
    if (!dragHandle_)
        return;

    dragHandle_->AddEventListener("dragstart", listener_.get(), false);
    dragHandle_->AddEventListener("drag", listener_.get(), false);
    dragHandle_->AddEventListener("dragend", listener_.get(), false);
}

void RmlMuMovablePanel::Unbind()
{
    if (dragHandle_)
    {
        dragHandle_->RemoveEventListener("dragstart", listener_.get(), false);
        dragHandle_->RemoveEventListener("drag", listener_.get(), false);
        dragHandle_->RemoveEventListener("dragend", listener_.get(), false);
    }

    panel_ = nullptr;
    dragHandle_ = nullptr;

    containerWidth_ = 0.0f;
    containerHeight_ = 0.0f;
    panelWidth_ = 0.0f;
    panelHeight_ = 0.0f;
    parentOffsetX_ = 0.0f;
    parentOffsetY_ = 0.0f;
    positionX_ = 0.0f;
    positionY_ = 0.0f;
    dragOffsetX_ = 0.0f;
    dragOffsetY_ = 0.0f;
    dragging_ = false;
    positionChanged_ = false;
}

void RmlMuMovablePanel::SetMetrics(
    float containerWidth, float containerHeight,
    float panelWidth, float panelHeight,
    float parentOffsetX, float parentOffsetY)
{
    const bool originChanged =
        parentOffsetX_ != parentOffsetX ||
        parentOffsetY_ != parentOffsetY;

    containerWidth_ = containerWidth;
    containerHeight_ = containerHeight;
    panelWidth_ = panelWidth;
    panelHeight_ = panelHeight;
    parentOffsetX_ = parentOffsetX;
    parentOffsetY_ = parentOffsetY;

    if (ClampPosition() || originChanged)
        ApplyPosition();
}

void RmlMuMovablePanel::SetPosition(float x, float y)
{
    if (positionX_ == x && positionY_ == y)
        return;

    positionX_ = x;
    positionY_ = y;
    (void)ClampPosition();
    ApplyPosition();
}

Rml::Vector2f RmlMuMovablePanel::GetPosition() const noexcept
{
    return {positionX_, positionY_};
}

bool RmlMuMovablePanel::IsDragging() const noexcept
{
    return dragging_;
}

void RmlMuMovablePanel::CancelDrag() noexcept
{
    dragging_ = false;
}

bool RmlMuMovablePanel::ConsumePositionChanged() noexcept
{
    const bool changed = positionChanged_;
    positionChanged_ = false;
    return changed;
}

bool RmlMuMovablePanel::ClampPosition() noexcept
{
    const float maxX = std::max(0.0f, containerWidth_ - panelWidth_);
    const float maxY = std::max(0.0f, containerHeight_ - panelHeight_);

    const float clampedX = std::clamp(positionX_, 0.0f, maxX);
    const float clampedY = std::clamp(positionY_, 0.0f, maxY);
    const bool changed =
        clampedX != positionX_ || clampedY != positionY_;

    positionX_ = clampedX;
    positionY_ = clampedY;
    return changed;
}

void RmlMuMovablePanel::ApplyPosition()
{
    if (!panel_)
        return;

    panel_->SetProperty("left", PixelValue(positionX_ - parentOffsetX_));
    panel_->SetProperty("top", PixelValue(positionY_ - parentOffsetY_));
    positionChanged_ = true;
}

void RmlMuMovablePanel::OnEvent(Rml::Event& event)
{
    const Rml::String& type = event.GetType();

    if (type == "dragstart")
    {
        dragging_ = true;

        const float mouseX = event.GetParameter<float>("mouse_x", 0.0f);
        dragOffsetX_ = mouseX - positionX_;

        const float mouseY = event.GetParameter<float>("mouse_y", 0.0f);
        dragOffsetY_ = mouseY - positionY_;
        return;
    }

    if (type == "drag")
    {
        if (!dragging_)
            return;

        const float mouseX = event.GetParameter<float>("mouse_x", 0.0f);
        positionX_ = mouseX - dragOffsetX_;

        const float mouseY = event.GetParameter<float>("mouse_y", 0.0f);
        positionY_ = mouseY - dragOffsetY_;

        (void)ClampPosition();
        ApplyPosition();
        return;
    }

    if (type == "dragend")
        dragging_ = false;
}

void RmlMuMovablePanel::Listener::ProcessEvent(Rml::Event& event)
{
    if (owner_)
        owner_->OnEvent(event);
}

} // namespace UI::Modern

#include "stdafx.h"
#include "UI/Modern/RmlMuScrollBar.h"

#include <RmlUi/Core/Box.h>
#include <RmlUi/Core/Element.h>
#include <RmlUi/Core/Event.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <utility>

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

RmlMuScrollBar::RmlMuScrollBar()
    : listener_(std::make_unique<Listener>(this))
{
}

RmlMuScrollBar::~RmlMuScrollBar()
{
    Unbind();
}

bool RmlMuScrollBar::Bind(Rml::Element* root)
{
    Unbind();

    if (!root || root->GetNumChildren() != 4)
        return false;

    root_ = root;
    track_ = root->GetChild(0);
    increment_ = root->GetChild(1);
    decrement_ = root->GetChild(2);
    thumb_ = root->GetChild(3);

    if (!track_ || !increment_ || !decrement_ || !thumb_)
    {
        Unbind();
        return false;
    }

    // Exact event topology recovered from the Debug build.
    track_->AddEventListener("click", listener_.get(), false);
    increment_->AddEventListener("click", listener_.get(), false);
    decrement_->AddEventListener("click", listener_.get(), false);
    thumb_->AddEventListener("dragstart", listener_.get(), false);
    thumb_->AddEventListener("drag", listener_.get(), false);
    thumb_->AddEventListener("dragend", listener_.get(), false);

    appliedThumbTop_.reset();
    appliedThumbHeight_.reset();
    appliedVisible_.reset();
    appliedIncrementEnabled_.reset();
    appliedDecrementEnabled_.reset();
    return true;
}

void RmlMuScrollBar::Unbind()
{
    if (track_)
        track_->RemoveEventListener("click", listener_.get(), false);
    if (increment_)
        increment_->RemoveEventListener("click", listener_.get(), false);
    if (decrement_)
        decrement_->RemoveEventListener("click", listener_.get(), false);
    if (thumb_)
    {
        thumb_->RemoveEventListener("dragstart", listener_.get(), false);
        thumb_->RemoveEventListener("drag", listener_.get(), false);
        thumb_->RemoveEventListener("dragend", listener_.get(), false);
    }

    root_ = nullptr;
    track_ = nullptr;
    increment_ = nullptr;
    decrement_ = nullptr;
    thumb_ = nullptr;

    position_ = 0;
    maximum_ = 0;
    visibleAmount_ = 0;
    pageStep_ = 0;
    visible_ = true;
    enabled_ = true;
    requestedPosition_.reset();

    dragPixelsPerPosition_ = 1.0f;
    dragStartMouseY_ = 0.0f;
    dragStartPosition_ = 0;
    dragging_ = false;

    appliedThumbTop_.reset();
    appliedThumbHeight_.reset();
    appliedVisible_.reset();
    appliedIncrementEnabled_.reset();
    appliedDecrementEnabled_.reset();
}

bool RmlMuScrollBar::SetState(
    std::size_t position,
    std::size_t maximum,
    std::size_t visibleAmount,
    std::size_t pageStep,
    bool visible,
    bool enabled)
{
    const std::size_t clamped = std::min(position, maximum);
    const bool changed =
        position_ != clamped ||
        maximum_ != maximum ||
        visibleAmount_ != visibleAmount ||
        pageStep_ != pageStep ||
        visible_ != visible ||
        enabled_ != enabled;

    position_ = clamped;
    maximum_ = maximum;
    visibleAmount_ = visibleAmount;
    pageStep_ = pageStep;
    visible_ = visible;
    enabled_ = enabled;

    if (!enabled_)
    {
        dragging_ = false;
        requestedPosition_.reset();
    }

    return Update() || changed;
}

std::size_t RmlMuScrollBar::GetPosition() const noexcept
{
    return position_;
}

std::size_t RmlMuScrollBar::GetMaximum() const noexcept
{
    return maximum_;
}

bool RmlMuScrollBar::IsDragging() const noexcept
{
    return dragging_;
}

std::optional<std::size_t> RmlMuScrollBar::ConsumeRequestedPosition()
{
    if (requestedPosition_)
        position_ = std::min(*requestedPosition_, maximum_);

    return std::exchange(requestedPosition_, std::nullopt);
}

void RmlMuScrollBar::RequestPosition(std::size_t position) noexcept
{
    requestedPosition_ = std::min(position, maximum_);
}

float RmlMuScrollBar::TrackTop() const
{
    return track_ ? track_->GetAbsoluteOffset(Rml::BoxArea::Border).y : 0.0f;
}

float RmlMuScrollBar::TrackHeight() const
{
    return track_ ? track_->GetBox().GetSize(Rml::BoxArea::Border).y : 0.0f;
}

float RmlMuScrollBar::ThumbTop() const
{
    return thumb_ ? thumb_->GetAbsoluteOffset(Rml::BoxArea::Border).y : 0.0f;
}

float RmlMuScrollBar::ThumbHeight() const
{
    return thumb_ ? thumb_->GetBox().GetSize(Rml::BoxArea::Border).y : 0.0f;
}

float RmlMuScrollBar::ThumbTravel() const
{
    return std::max(0.0f, TrackHeight() - ThumbHeight());
}

float RmlMuScrollBar::PositionToOffset(std::size_t position) const
{
    if (maximum_ == 0)
        return 0.0f;

    return ThumbTravel() *
        (static_cast<float>(std::min(position, maximum_)) /
         static_cast<float>(maximum_));
}

std::size_t RmlMuScrollBar::DragDeltaToPosition(float deltaY) const
{
    if (maximum_ == 0)
        return 0;

    const float travel = ThumbTravel();
    if (travel <= 0.0f)
        return dragStartPosition_;

    // Debug helper arithmetic rounds the logical delta before clamping.
    const float positionsPerPixel =
        static_cast<float>(maximum_) / travel;
    const auto logicalDelta = static_cast<long long>(
        std::llround(deltaY * positionsPerPixel));

    long long candidate =
        static_cast<long long>(dragStartPosition_) + logicalDelta;
    candidate = std::clamp<long long>(
        candidate, 0, static_cast<long long>(maximum_));
    return static_cast<std::size_t>(candidate);
}

bool RmlMuScrollBar::SetPixelProperty(
    Rml::Element* element, const char* name, float value)
{
    return element && element->SetProperty(name, PixelValue(value));
}

bool RmlMuScrollBar::Update()
{
    if (!root_ || !track_ || !increment_ || !decrement_ || !thumb_)
        return false;

    bool changed = false;

    const bool shown = visible_ && maximum_ > 0;
    if (!appliedVisible_ || *appliedVisible_ != shown)
    {
        root_->SetProperty("display", shown ? "block" : "none");
        appliedVisible_ = shown;
        changed = true;
    }

    const bool decrementEnabled = enabled_ && position_ > 0;
    const bool incrementEnabled = enabled_ && position_ < maximum_;

    if (!appliedDecrementEnabled_ ||
        *appliedDecrementEnabled_ != decrementEnabled)
    {
        decrement_->SetClass("disabled", !decrementEnabled);
        decrement_->SetProperty(
            "pointer-events", decrementEnabled ? "auto" : "none");
        appliedDecrementEnabled_ = decrementEnabled;
        changed = true;
    }

    if (!appliedIncrementEnabled_ ||
        *appliedIncrementEnabled_ != incrementEnabled)
    {
        increment_->SetClass("disabled", !incrementEnabled);
        increment_->SetProperty(
            "pointer-events", incrementEnabled ? "auto" : "none");
        appliedIncrementEnabled_ = incrementEnabled;
        changed = true;
    }

    if (!shown)
        return changed;

    const float trackHeight = TrackHeight();
    const float minThumbHeight = std::max(1.0f, ThumbHeight());

    float thumbHeight = trackHeight;
    if (maximum_ > 0)
    {
        const std::size_t total = maximum_ + visibleAmount_;
        if (total > 0)
        {
            thumbHeight = trackHeight *
                (static_cast<float>(visibleAmount_) /
                 static_cast<float>(total));
        }
    }
    thumbHeight = std::clamp(thumbHeight, minThumbHeight, trackHeight);

    // Set height first because the available travel depends on the rendered
    // thumb size. The next RmlUi layout update will make GetBox reflect it.
    if (!appliedThumbHeight_ ||
        std::abs(*appliedThumbHeight_ - thumbHeight) > 0.001f)
    {
        (void)SetPixelProperty(thumb_, "height", thumbHeight);
        appliedThumbHeight_ = thumbHeight;
        changed = true;
    }

    const float travel = std::max(0.0f, trackHeight - thumbHeight);
    const float top = maximum_ == 0
        ? 0.0f
        : travel * (static_cast<float>(position_) /
                    static_cast<float>(maximum_));

    if (!appliedThumbTop_ ||
        std::abs(*appliedThumbTop_ - top) > 0.001f)
    {
        (void)SetPixelProperty(thumb_, "top", top);
        appliedThumbTop_ = top;
        changed = true;
    }

    dragPixelsPerPosition_ =
        maximum_ == 0 ? 1.0f :
        travel / static_cast<float>(maximum_);
    if (!(dragPixelsPerPosition_ > 0.0f))
        dragPixelsPerPosition_ = 1.0f;

    return changed;
}

void RmlMuScrollBar::OnEvent(Rml::Event& event)
{
    Rml::Element* current = event.GetCurrentElement();
    const Rml::String& type = event.GetType();

    if (type == "click")
    {
        if (!enabled_)
            return;

        if (current == decrement_ && position_ > 0)
        {
            RequestPosition(position_ - 1);
        }
        else if (current == increment_ && position_ < maximum_)
        {
            RequestPosition(position_ + 1);
        }
        else if (current == track_ && maximum_ > 0)
        {
            const float mouseY =
                event.GetParameter<float>("mouse_y", 0.0f);
            const float thumbCenter =
                ThumbTop() + ThumbHeight() * 0.5f;
            const std::size_t page =
                pageStep_ > 0
                    ? pageStep_
                    : std::max<std::size_t>(1, visibleAmount_);

            if (mouseY < thumbCenter)
            {
                RequestPosition(
                    position_ > page ? position_ - page : 0);
            }
            else if (mouseY > thumbCenter)
            {
                RequestPosition(
                    std::min(maximum_, position_ + page));
            }
        }

        event.StopPropagation();
        return;
    }

    if (type == "dragstart")
    {
        if (current == thumb_ && enabled_ && maximum_ > 0)
        {
            dragging_ = true;
            dragStartMouseY_ =
                event.GetParameter<float>("mouse_y", 0.0f);
            dragStartPosition_ = position_;
        }
        event.StopPropagation();
        return;
    }

    if (type == "drag")
    {
        if (dragging_)
        {
            const float mouseY =
                event.GetParameter<float>("mouse_y", dragStartMouseY_);
            RequestPosition(
                DragDeltaToPosition(mouseY - dragStartMouseY_));
        }
        event.StopPropagation();
        return;
    }

    if (type == "dragend")
    {
        dragging_ = false;
        event.StopPropagation();
    }
}

void RmlMuScrollBar::Listener::ProcessEvent(Rml::Event& event)
{
    if (owner_)
        owner_->OnEvent(event);
}

} // namespace UI::Modern

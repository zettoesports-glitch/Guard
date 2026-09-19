#include "stdafx.h"
#include "UI/Modern/PC/HUD/RmlTopMenuLayer.h"

#include "UI/Modern/RmlDocumentHost.h"
#include "UI/Modern/RmlMuButton.h"
#include "UI/Modern/RmlUiDesign.h"

#include <RmlUi/Core/Core.h>
#include <RmlUi/Core/Element.h>
#include <RmlUi/Core/ElementDocument.h>
#include <RmlUi/Core/FileInterface.h>
#include <RmlUi/Core/StreamMemory.h>

#include <algorithm>
#include <array>
#include <cstdio>
#include <sstream>
#include <string>
#include <utility>

namespace UI::Modern::PC::HUD
{

namespace
{
constexpr const char* kDocumentPath = "Data/UI/PC/HUD/top_menu.rml";

Rml::String PixelValue(float value)
{
    std::array<char, 64> buffer{};
    std::snprintf(
        buffer.data(), buffer.size(), "%.3fpx",
        static_cast<double>(value));
    return Rml::String(buffer.data());
}

Rml::String ScaleValue(float value)
{
    std::array<char, 64> buffer{};
    std::snprintf(
        buffer.data(), buffer.size(), "scale(%.6f)",
        static_cast<double>(value));
    return Rml::String(buffer.data());
}

template <typename T>
bool ParseScalar(
    const RmlUiDesign::Values& values,
    const char* key,
    T& output)
{
    const auto it = values.find(key);
    if (it == values.end())
        return false;

    std::istringstream stream(it->second);
    T parsed{};
    if (!(stream >> parsed))
        return false;

    output = parsed;
    return true;
}

RmlUiDesign::Values LoadDesignValues()
{
    Rml::FileInterface* files = Rml::GetFileInterface();
    if (!files)
        return {};

    Rml::String contents;
    if (!files->LoadFile(kDocumentPath, contents) || contents.empty())
        return {};

    Rml::StreamMemory stream(
        reinterpret_cast<const Rml::byte*>(contents.data()),
        contents.size());
    return RmlUiDesign::Parse(&stream);
}

Rml::Element* RequiredElement(
    Rml::ElementDocument* document,
    const char* id)
{
    return document ? document->GetElementById(id) : nullptr;
}

} // namespace

class RmlTopMenuLayer::Impl
{
public:
    struct Design
    {
        float resizeStageHeight = 700.0f;
        float smallStageScale = 0.83f;
        float frameWidth = 341.0f;
        float frameHeight = 39.0f;
        float referenceWidth = 640.0f;
        float referenceHeight = 480.0f;
    };

    Impl()
        : host_(kDocumentPath)
    {
    }

    ~Impl()
    {
        Release();
    }

    [[nodiscard]] bool Load(bool show)
    {
        if (host_.IsLoaded())
        {
            if (show)
                return Show();
            return true;
        }

        if (!host_.Load(show))
            return false;

        document_ = host_.GetDocument();
        if (!document_)
            return false;

        ReadDesign();
        if (!BindElements())
        {
            Release();
            return false;
        }

        if (show)
            state_.visible = true;

        return ApplyState(state_);
    }

    [[nodiscard]] bool Show()
    {
        if (!host_.IsLoaded() && !Load(false))
            return false;

        state_.visible = true;
        const bool shown = host_.Show();
        if (shown)
            (void)ApplyState(state_);
        return shown;
    }

    [[nodiscard]] bool Hide()
    {
        state_.visible = false;
        if (root_)
            root_->SetProperty("display", "none");
        return host_.Hide();
    }

    void Release()
    {
        optionButton_.Unbind();
        actionButton_.Unbind();

        root_ = nullptr;
        wingBack_ = nullptr;
        wingControls_ = nullptr;
        mapFrame_ = nullptr;
        mapName_ = nullptr;
        position_ = nullptr;
        optionElement_ = nullptr;
        actionElement_ = nullptr;
        document_ = nullptr;
        pendingAction_.reset();

        host_.Release();
    }

    [[nodiscard]] bool IsLoaded() const noexcept
    {
        return host_.IsLoaded() && document_ != nullptr;
    }

    [[nodiscard]] bool ApplyState(const State& state)
    {
        state_ = state;
        if (!IsLoaded())
            return false;

        const bool visible =
            state_.visible &&
            state_.viewportWidth > 0 &&
            state_.viewportHeight > 0;

        root_->SetProperty("display", visible ? "block" : "none");
        if (!visible)
            return true;

        const float scale =
            static_cast<float>(state_.viewportHeight) <
                    design_.resizeStageHeight
                ? design_.smallStageScale
                : 1.0f;

        // The recovered layer uses an authored 341x39 top frame. Keep it
        // centered horizontally and pinned to the top edge after scaling.
        const float width = design_.frameWidth * scale;
        const float left = std::max(
            0.0f,
            (static_cast<float>(state_.viewportWidth) - width) * 0.5f);

        root_->SetProperty("left", PixelValue(left));
        root_->SetProperty("top", "0px");
        root_->SetProperty("transform", ScaleValue(scale));

        mapName_->SetInnerRML(state_.mapName);
        position_->SetInnerRML(
            std::to_string(state_.mapX) + ", " +
            std::to_string(state_.mapY));

        optionButton_.SetEnabled(state_.optionEnabled);
        actionButton_.SetEnabled(state_.actionEnabled);
        (void)optionButton_.Update();
        (void)actionButton_.Update();

        // Exact Debug class contract for the helper action.
        actionElement_->SetClass("start", !state_.helperRunning);
        actionElement_->SetClass("stop", state_.helperRunning);

        document_->UpdateDocument();
        return true;
    }

    [[nodiscard]] bool Update()
    {
        if (!IsLoaded())
            return false;

        bool changed = false;
        changed |= optionButton_.Update();
        changed |= actionButton_.Update();

        if (optionButton_.ConsumeClicked())
        {
            pendingAction_ = Action::OpenOptions;
            changed = true;
        }

        if (actionButton_.ConsumeClicked())
        {
            pendingAction_ = Action::ToggleHelper;
            changed = true;
        }

        return changed;
    }

    [[nodiscard]] std::optional<Action> ConsumeAction()
    {
        return std::exchange(pendingAction_, std::nullopt);
    }

private:
    void ReadDesign()
    {
        const RmlUiDesign::Values values = LoadDesignValues();
        (void)ParseScalar(
            values, "TopMenu-ResizeStageHeight",
            design_.resizeStageHeight);
        (void)ParseScalar(
            values, "TopMenu-SmallStageScale",
            design_.smallStageScale);
        (void)ParseScalar(
            values, "TopMenu-FrameWidth",
            design_.frameWidth);
        (void)ParseScalar(
            values, "TopMenu-FrameHeight",
            design_.frameHeight);
        (void)ParseScalar(
            values, "TopMenu-ReferenceWidth",
            design_.referenceWidth);
        (void)ParseScalar(
            values, "TopMenu-ReferenceHeight",
            design_.referenceHeight);
    }

    [[nodiscard]] bool BindElements()
    {
        root_ = RequiredElement(document_, "top-menu");
        wingBack_ =
            RequiredElement(document_, "top-menu-wing-back");
        wingControls_ =
            RequiredElement(document_, "top-menu-wing-controls");
        mapFrame_ =
            RequiredElement(document_, "top-menu-map-frame");
        mapName_ =
            RequiredElement(document_, "top-menu-map-name");
        position_ =
            RequiredElement(document_, "top-menu-position");
        optionElement_ =
            RequiredElement(document_, "top-menu-option");
        actionElement_ =
            RequiredElement(document_, "top-menu-action");

        if (!root_ || !wingBack_ || !wingControls_ || !mapFrame_ ||
            !mapName_ || !position_ || !optionElement_ || !actionElement_)
            return false;

        optionButton_.Bind(optionElement_);
        actionButton_.Bind(actionElement_);
        return true;
    }

    RmlDocumentHost host_;
    Rml::ElementDocument* document_ = nullptr;

    Rml::Element* root_ = nullptr;
    Rml::Element* wingBack_ = nullptr;
    Rml::Element* wingControls_ = nullptr;
    Rml::Element* mapFrame_ = nullptr;
    Rml::Element* mapName_ = nullptr;
    Rml::Element* position_ = nullptr;
    Rml::Element* optionElement_ = nullptr;
    Rml::Element* actionElement_ = nullptr;

    RmlMuButton optionButton_;
    RmlMuButton actionButton_;

    Design design_;
    State state_;
    std::optional<Action> pendingAction_;
};

RmlTopMenuLayer::RmlTopMenuLayer()
    : m_impl(std::make_unique<Impl>())
{
}

RmlTopMenuLayer::~RmlTopMenuLayer() = default;
RmlTopMenuLayer::RmlTopMenuLayer(RmlTopMenuLayer&&) noexcept = default;
RmlTopMenuLayer& RmlTopMenuLayer::operator=(
    RmlTopMenuLayer&&) noexcept = default;

bool RmlTopMenuLayer::Load(bool show)
{
    return m_impl && m_impl->Load(show);
}

bool RmlTopMenuLayer::Show()
{
    return m_impl && m_impl->Show();
}

bool RmlTopMenuLayer::Hide()
{
    return m_impl && m_impl->Hide();
}

void RmlTopMenuLayer::Release()
{
    if (m_impl)
        m_impl->Release();
}

bool RmlTopMenuLayer::IsLoaded() const noexcept
{
    return m_impl && m_impl->IsLoaded();
}

bool RmlTopMenuLayer::ApplyState(const State& state)
{
    return m_impl && m_impl->ApplyState(state);
}

bool RmlTopMenuLayer::Update()
{
    return m_impl && m_impl->Update();
}

std::optional<RmlTopMenuLayer::Action>
RmlTopMenuLayer::ConsumeAction()
{
    return m_impl
        ? m_impl->ConsumeAction()
        : std::nullopt;
}

} // namespace UI::Modern::PC::HUD

#include "stdafx.h"
#include "UI/Modern/PC/Command/RmlCommandWindowPanel.h"

#include "UI/Modern/RmlDocumentHost.h"
#include "UI/Modern/RmlMuButton.h"
#include "UI/Modern/RmlMuMovablePanel.h"
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
#include <utility>

namespace UI::Modern::PC::Command
{

namespace
{
constexpr const char* kDocumentPath =
    "Data/UI/PC/Command/command_window.rml";

Rml::String PixelValue(float value)
{
    std::array<char, 64> buffer{};
    std::snprintf(
        buffer.data(), buffer.size(), "%.3fpx",
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
    Rml::ElementDocument* document, const Rml::String& id)
{
    return document ? document->GetElementById(id) : nullptr;
}

} // namespace

class RmlCommandWindowPanel::Impl
{
public:
    struct Design
    {
        float panelWidth = 327.0f;
        float panelHeight = 639.0f;
        float referenceWidth = 640.0f;
        float referenceHeight = 480.0f;
        float buttonX = 101.0f;
        float buttonY = 102.0f;
        float buttonStep = 44.0f;
        float buttonWidth = 128.000259f;
        float buttonHeight = 35.0f;
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
        for (auto& button : buttons_)
            button.Unbind();
        closeButton_.Unbind();
        mover_.Unbind();

        document_ = nullptr;
        root_ = nullptr;
        title_ = nullptr;
        drag_ = nullptr;
        closeElement_ = nullptr;
        buttonElements_.fill(nullptr);
        labelElements_.fill(nullptr);
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
        if (state_.selectedCommand &&
            *state_.selectedCommand >= CommandCount)
            state_.selectedCommand.reset();

        if (!IsLoaded())
            return false;

        const bool visible =
            state_.visible &&
            state_.viewportWidth > 0 &&
            state_.viewportHeight > 0;

        root_->SetProperty(
            "display", visible ? "block" : "none");
        if (!visible)
            return true;

        ApplyLayout();
        title_->SetInnerRML(state_.title);

        for (std::size_t i = 0; i < CommandCount; ++i)
        {
            const Command& command = state_.commands[i];

            labelElements_[i]->SetInnerRML(command.label);
            buttons_[i].SetVisible(command.visible);
            buttons_[i].SetEnabled(
                command.visible && command.enabled);
            (void)buttons_[i].Update();

            buttonElements_[i]->SetClass(
                "selected",
                state_.selectedCommand &&
                *state_.selectedCommand == i);
        }

        (void)closeButton_.Update();
        document_->UpdateDocument();
        return true;
    }

    [[nodiscard]] bool Update()
    {
        if (!IsLoaded())
            return false;

        bool changed = false;

        for (std::size_t i = 0; i < CommandCount; ++i)
        {
            changed |= buttons_[i].Update();
            if (buttons_[i].ConsumeClicked())
            {
                state_.selectedCommand = i;
                pendingAction_ =
                    Action{ActionType::SelectCommand, i};
                SyncSelection();
                changed = true;
            }
        }

        changed |= closeButton_.Update();
        if (closeButton_.ConsumeClicked())
        {
            pendingAction_ =
                Action{ActionType::Close, 0};
            changed = true;
        }

        changed |= mover_.ConsumePositionChanged();
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
            values, "RmlCommandWindowPanel-PanelWidth",
            design_.panelWidth);
        (void)ParseScalar(
            values, "RmlCommandWindowPanel-PanelHeight",
            design_.panelHeight);
        (void)ParseScalar(
            values, "RmlCommandWindowPanel-ReferenceWidth",
            design_.referenceWidth);
        (void)ParseScalar(
            values, "RmlCommandWindowPanel-ReferenceHeight",
            design_.referenceHeight);
        (void)ParseScalar(
            values, "RmlCommandWindowPanel-ButtonX",
            design_.buttonX);
        (void)ParseScalar(
            values, "RmlCommandWindowPanel-ButtonY",
            design_.buttonY);
        (void)ParseScalar(
            values, "RmlCommandWindowPanel-ButtonStep",
            design_.buttonStep);
        (void)ParseScalar(
            values, "RmlCommandWindowPanel-ButtonWidth",
            design_.buttonWidth);
        (void)ParseScalar(
            values, "RmlCommandWindowPanel-ButtonHeight",
            design_.buttonHeight);
    }

    [[nodiscard]] bool BindElements()
    {
        root_ =
            RequiredElement(document_, "command-window");
        title_ =
            RequiredElement(document_, "command-window-title");
        drag_ =
            RequiredElement(document_, "command-window-drag");
        closeElement_ =
            RequiredElement(document_, "command-window-close");

        if (!root_ || !title_ || !drag_ || !closeElement_)
            return false;

        for (std::size_t i = 0; i < CommandCount; ++i)
        {
            const std::string suffix = std::to_string(i);
            buttonElements_[i] =
                RequiredElement(
                    document_, "command-button-" + suffix);
            labelElements_[i] =
                RequiredElement(
                    document_, "command-label-" + suffix);

            if (!buttonElements_[i] || !labelElements_[i])
                return false;

            buttons_[i].Bind(buttonElements_[i]);
        }

        closeButton_.Bind(closeElement_);
        mover_.Bind(root_, drag_);
        return true;
    }

    void ApplyLayout()
    {
        root_->SetProperty(
            "width", PixelValue(design_.panelWidth));
        root_->SetProperty(
            "height", PixelValue(design_.panelHeight));

        for (std::size_t i = 0; i < CommandCount; ++i)
        {
            buttonElements_[i]->SetProperty(
                "left", PixelValue(design_.buttonX));
            buttonElements_[i]->SetProperty(
                "top",
                PixelValue(
                    design_.buttonY +
                    design_.buttonStep *
                        static_cast<float>(i)));
            buttonElements_[i]->SetProperty(
                "width", PixelValue(design_.buttonWidth));
            buttonElements_[i]->SetProperty(
                "height", PixelValue(design_.buttonHeight));
        }

        mover_.SetMetrics(
            static_cast<float>(state_.viewportWidth),
            static_cast<float>(state_.viewportHeight),
            design_.panelWidth,
            design_.panelHeight,
            0.0f,
            0.0f);

        if (state_.applyPosition)
            mover_.SetPosition(
                state_.positionX, state_.positionY);
    }

    void SyncSelection()
    {
        for (std::size_t i = 0; i < CommandCount; ++i)
        {
            buttonElements_[i]->SetClass(
                "selected",
                state_.selectedCommand &&
                *state_.selectedCommand == i);
        }
    }

    RmlDocumentHost host_;
    Rml::ElementDocument* document_ = nullptr;

    Rml::Element* root_ = nullptr;
    Rml::Element* title_ = nullptr;
    Rml::Element* drag_ = nullptr;
    Rml::Element* closeElement_ = nullptr;
    std::array<Rml::Element*, CommandCount> buttonElements_{};
    std::array<Rml::Element*, CommandCount> labelElements_{};

    std::array<RmlMuButton, CommandCount> buttons_{};
    RmlMuButton closeButton_;
    RmlMuMovablePanel mover_;

    Design design_;
    State state_;
    std::optional<Action> pendingAction_;
};

RmlCommandWindowPanel::RmlCommandWindowPanel()
    : m_impl(std::make_unique<Impl>())
{
}

RmlCommandWindowPanel::~RmlCommandWindowPanel() = default;
RmlCommandWindowPanel::RmlCommandWindowPanel(
    RmlCommandWindowPanel&&) noexcept = default;
RmlCommandWindowPanel& RmlCommandWindowPanel::operator=(
    RmlCommandWindowPanel&&) noexcept = default;

bool RmlCommandWindowPanel::Load(bool show)
{
    return m_impl && m_impl->Load(show);
}

bool RmlCommandWindowPanel::Show()
{
    return m_impl && m_impl->Show();
}

bool RmlCommandWindowPanel::Hide()
{
    return m_impl && m_impl->Hide();
}

void RmlCommandWindowPanel::Release()
{
    if (m_impl)
        m_impl->Release();
}

bool RmlCommandWindowPanel::IsLoaded() const noexcept
{
    return m_impl && m_impl->IsLoaded();
}

bool RmlCommandWindowPanel::ApplyState(const State& state)
{
    return m_impl && m_impl->ApplyState(state);
}

bool RmlCommandWindowPanel::Update()
{
    return m_impl && m_impl->Update();
}

std::optional<RmlCommandWindowPanel::Action>
RmlCommandWindowPanel::ConsumeAction()
{
    return m_impl
        ? m_impl->ConsumeAction()
        : std::nullopt;
}

} // namespace UI::Modern::PC::Command

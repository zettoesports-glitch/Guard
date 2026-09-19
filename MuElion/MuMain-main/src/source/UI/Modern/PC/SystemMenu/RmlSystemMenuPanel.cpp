#include "stdafx.h"
#include "UI/Modern/PC/SystemMenu/RmlSystemMenuPanel.h"

#include "UI/Modern/RmlDocumentHost.h"
#include "UI/Modern/RmlMuButton.h"
#include "UI/Modern/RmlUiDesign.h"

#include <RmlUi/Core/Core.h>
#include <RmlUi/Core/Element.h>
#include <RmlUi/Core/ElementDocument.h>
#include <RmlUi/Core/Event.h>
#include <RmlUi/Core/EventListener.h>
#include <RmlUi/Core/FileInterface.h>
#include <RmlUi/Core/Input.h>
#include <RmlUi/Core/StreamMemory.h>

#include <algorithm>
#include <array>
#include <cstdio>
#include <sstream>
#include <utility>
#include <vector>

namespace UI::Modern::PC::SystemMenu
{

namespace
{
constexpr const char* kDocumentPath =
    "Data/UI/PC/SystemMenu/system_menu.rml";

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

Rml::Element* ElementById(
    Rml::ElementDocument* document, const char* id)
{
    return document ? document->GetElementById(id) : nullptr;
}

void SetText(Rml::Element* element, const std::string& text)
{
    if (!element)
        return;
    element->SetInnerRML("");
    if (Rml::ElementDocument* document = element->GetOwnerDocument())
        element->AppendChild(document->CreateTextNode(text));
}

} // namespace

class RmlSystemMenuPanel::Impl final : public Rml::EventListener
{
public:
    struct Design
    {
        float width = 306.0f;
        float fullHeight = 290.000336f;
        float loginHeight = 242.050336f;
        float firstSlotX = 93.0f;
        float firstSlotY = 65.0f;
        float slotStep = 47.95f;
        float dividerY = 228.0f;
        float closeY = 243.0f;
        float compactOffset = 47.95f;
    };

    Impl() : host_(kDocumentPath) {}
    ~Impl() override { Release(); }

    [[nodiscard]] bool Load(bool show)
    {
        if (host_.IsLoaded())
        {
            if (show) return Show();
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

        hostVisible_ = show;
        return ApplyState(state_);
    }

    [[nodiscard]] bool Show()
    {
        if (!host_.IsLoaded() && !Load(false))
            return false;

        hostVisible_ = true;
        const bool result = host_.Show();
        if (result) (void)ApplyState(state_);
        return result;
    }

    [[nodiscard]] bool Hide()
    {
        hostVisible_ = false;
        if (root_) root_->SetProperty("display", "none");
        return host_.Hide();
    }

    void Release()
    {
        if (root_)
            root_->RemoveEventListener("keydown", this, false);

        for (auto& button : buttons_) button.Unbind();

        document_ = nullptr;
        root_ = title_ = divider_ = bottomDivider_ = nullptr;
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
        if (!IsLoaded())
            return false;

        const bool visible =
            hostVisible_ && state_.visible &&
            state_.viewportWidth > 0 && state_.viewportHeight > 0;

        root_->SetProperty("display", visible ? "block" : "none");
        if (!visible)
            return true;

        SetText(title_, state_.title);

        const std::array<const ButtonState*, 5> states{
            &state_.exit, &state_.server, &state_.character,
            &state_.option, &state_.close};

        for (std::size_t i = 0; i < states.size(); ++i)
        {
            SetText(labelElements_[i], states[i]->label);
            buttons_[i].SetVisible(states[i]->visible);
            buttons_[i].SetEnabled(
                states[i]->visible && states[i]->enabled);
            (void)buttons_[i].Update();
        }

        ApplyLayout();
        document_->UpdateDocument();
        return true;
    }

    [[nodiscard]] bool Update()
    {
        if (!IsLoaded())
            return false;

        bool changed = false;
        static constexpr std::array<ActionType, 5> actions{
            ActionType::Exit,
            ActionType::Server,
            ActionType::Character,
            ActionType::Option,
            ActionType::Close};

        for (std::size_t i = 0; i < buttons_.size(); ++i)
        {
            changed |= buttons_[i].Update();
            if (buttons_[i].ConsumeClicked())
            {
                pendingAction_ = actions[i];
                changed = true;
            }
        }

        return changed;
    }

    [[nodiscard]] std::optional<ActionType> ConsumeAction()
    {
        return std::exchange(pendingAction_, std::nullopt);
    }

    void ProcessEvent(Rml::Event& event) override
    {
        if (event.GetType() != "keydown")
            return;

        const auto key = static_cast<Rml::Input::KeyIdentifier>(
            event.GetParameter<int>("key_identifier", 0));
        if (key == Rml::Input::KI_ESCAPE && state_.close.enabled)
        {
            pendingAction_ = ActionType::Close;
            event.StopPropagation();
        }
    }

private:
    void ReadDesign()
    {
        const RmlUiDesign::Values values = LoadDesignValues();
        (void)ParseScalar(values, "SystemMenu-Width", design_.width);
        (void)ParseScalar(values, "SystemMenu-FullHeight", design_.fullHeight);
        (void)ParseScalar(values, "SystemMenu-LoginHeight", design_.loginHeight);
        (void)ParseScalar(values, "SystemMenu-DividerY", design_.dividerY);
        (void)ParseScalar(values, "SystemMenu-CloseY", design_.closeY);
        (void)ParseScalar(values, "SystemMenu-CompactOffset", design_.compactOffset);
    }

    [[nodiscard]] bool BindElements()
    {
        root_ = ElementById(document_, "system-menu");
        title_ = ElementById(document_, "title");
        divider_ = ElementById(document_, "title-divider");
        bottomDivider_ = ElementById(document_, "bottom-divider");

        static constexpr std::array<const char*, 5> buttonIds{
            "exit-button", "server-button", "character-button",
            "option-button", "close-button"};
        static constexpr std::array<const char*, 5> labelIds{
            "exit-label", "server-label", "character-label",
            "option-label", "close-label"};

        if (!root_ || !title_ || !divider_ || !bottomDivider_)
            return false;

        for (std::size_t i = 0; i < buttonIds.size(); ++i)
        {
            buttonElements_[i] = ElementById(document_, buttonIds[i]);
            labelElements_[i] = ElementById(document_, labelIds[i]);
            if (!buttonElements_[i] || !labelElements_[i])
                return false;
            buttons_[i].Bind(buttonElements_[i]);
        }

        root_->SetAttribute("tabindex", 0);
        root_->AddEventListener("keydown", this, false);
        return true;
    }

    void ApplyLayout()
    {
        std::vector<std::size_t> primary;
        for (std::size_t i = 0; i < 4; ++i)
        {
            if (buttons_[i].IsVisible())
                primary.push_back(i);
        }

        const std::size_t count = primary.size();
        const bool compact = count <= 2;
        const float height =
            compact ? design_.loginHeight : design_.fullHeight;

        root_->SetProperty("width", PixelValue(design_.width));
        root_->SetProperty("height", PixelValue(height));
        root_->SetProperty(
            "left",
            PixelValue(std::max(
                0.0f,
                (static_cast<float>(state_.viewportWidth) -
                 design_.width) * 0.5f)));
        root_->SetProperty(
            "top",
            PixelValue(std::max(
                0.0f,
                (static_cast<float>(state_.viewportHeight) -
                 height) * 0.5f)));

        for (std::size_t visibleIndex = 0;
             visibleIndex < primary.size(); ++visibleIndex)
        {
            Rml::Element* button = buttonElements_[primary[visibleIndex]];
            button->SetProperty("left", PixelValue(design_.firstSlotX));
            button->SetProperty(
                "top",
                PixelValue(
                    design_.firstSlotY +
                    design_.slotStep *
                        static_cast<float>(visibleIndex)));
        }

        const float offset = compact ? design_.compactOffset : 0.0f;
        bottomDivider_->SetProperty(
            "top", PixelValue(design_.dividerY - offset));
        buttonElements_[4]->SetProperty(
            "top", PixelValue(design_.closeY - offset));
    }

    RmlDocumentHost host_;
    Rml::ElementDocument* document_ = nullptr;
    bool hostVisible_ = false;

    Rml::Element* root_ = nullptr;
    Rml::Element* title_ = nullptr;
    Rml::Element* divider_ = nullptr;
    Rml::Element* bottomDivider_ = nullptr;
    std::array<Rml::Element*, 5> buttonElements_{};
    std::array<Rml::Element*, 5> labelElements_{};
    std::array<RmlMuButton, 5> buttons_{};

    Design design_;
    State state_;
    std::optional<ActionType> pendingAction_;
};

RmlSystemMenuPanel::RmlSystemMenuPanel()
    : m_impl(std::make_unique<Impl>())
{
}
RmlSystemMenuPanel::~RmlSystemMenuPanel() = default;
RmlSystemMenuPanel::RmlSystemMenuPanel(
    RmlSystemMenuPanel&&) noexcept = default;
RmlSystemMenuPanel& RmlSystemMenuPanel::operator=(
    RmlSystemMenuPanel&&) noexcept = default;

bool RmlSystemMenuPanel::Load(bool show)
{
    return m_impl && m_impl->Load(show);
}
bool RmlSystemMenuPanel::Show()
{
    return m_impl && m_impl->Show();
}
bool RmlSystemMenuPanel::Hide()
{
    return m_impl && m_impl->Hide();
}
void RmlSystemMenuPanel::Release()
{
    if (m_impl) m_impl->Release();
}
bool RmlSystemMenuPanel::IsLoaded() const noexcept
{
    return m_impl && m_impl->IsLoaded();
}
bool RmlSystemMenuPanel::ApplyState(const State& state)
{
    return m_impl && m_impl->ApplyState(state);
}
bool RmlSystemMenuPanel::Update()
{
    return m_impl && m_impl->Update();
}
std::optional<RmlSystemMenuPanel::ActionType>
RmlSystemMenuPanel::ConsumeAction()
{
    return m_impl ? m_impl->ConsumeAction() : std::nullopt;
}

} // namespace UI::Modern::PC::SystemMenu

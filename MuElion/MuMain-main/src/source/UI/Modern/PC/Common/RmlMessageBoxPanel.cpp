#include "stdafx.h"
#include "UI/Modern/PC/Common/RmlMessageBoxPanel.h"

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
#include <RmlUi/Core/Elements/ElementFormControlInput.h>

#include <algorithm>
#include <array>
#include <cstdio>
#include <sstream>
#include <utility>

namespace UI::Modern::PC::Common
{

namespace
{
constexpr const char* kDocumentPath =
    "Data/UI/PC/Common/message_box.rml";

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

class RmlMessageBoxPanel::Impl final : public Rml::EventListener
{
public:
    struct Design
    {
        float panelWidth = 352.0f;
        float panelHeight = 210.0f;
        float buttonWidth = 70.0f;
        float buttonHeight = 30.0f;
        float singleButtonX = 141.0f;
        float okButtonX = 96.0f;
        float cancelButtonX = 186.0f;
        float buttonY = 164.0f;

        float cautionWidth = 308.0f;
        float cautionHeight = 190.0f;
        float cautionButtonWidth = 75.0f;
        float cautionButtonHeight = 35.0f;
        float cautionSingleButtonX = 109.0f;
        float cautionOkButtonX = 69.0f;
        float cautionCancelButtonX = 149.0f;
        float cautionBottomY = 136.0f;
        float cautionMessageHeight = 84.0f;

        int passwordMaxLength = 20;
        int numberMaxLength = 8;
        int textMaxLength = 10;
    };

    Impl() : host_(kDocumentPath) {}
    ~Impl() override { Release(); }

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

        hostVisible_ = show;
        return ApplyState(state_);
    }

    [[nodiscard]] bool Show()
    {
        if (!host_.IsLoaded() && !Load(false))
            return false;

        hostVisible_ = true;
        const bool result = host_.Show();
        if (result)
            (void)ApplyState(state_);
        return result;
    }

    [[nodiscard]] bool Hide()
    {
        hostVisible_ = false;
        if (root_)
            root_->SetProperty("display", "none");
        return host_.Hide();
    }

    void Release()
    {
        if (root_)
            root_->RemoveEventListener("keydown", this, false);

        okButton_.Unbind();
        cancelButton_.Unbind();

        root_ = nullptr;
        title_ = nullptr;
        message_ = nullptr;
        content_ = nullptr;
        input_ = nullptr;
        okElement_ = nullptr;
        cancelElement_ = nullptr;
        okLabel_ = nullptr;
        cancelLabel_ = nullptr;
        bottom_ = nullptr;

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
            hostVisible_ &&
            state_.visible &&
            state_.viewportWidth > 0 &&
            state_.viewportHeight > 0;

        root_->SetProperty("display", visible ? "block" : "none");
        if (!visible)
            return true;

        root_->SetClass("s16-caution", state_.cautionStyle);
        root_->SetClass("two-buttons", state_.twoButtons);
        root_->SetClass("message-only", state_.messageOnly);

        ApplyLayout();
        ApplyMessage();

        const bool showTitle = !state_.title.empty();
        title_->SetProperty("display", showTitle ? "block" : "none");
        SetText(title_, state_.title);

        ConfigureInput();

        okButton_.SetVisible(true);
        okButton_.SetEnabled(state_.okEnabled);
        cancelButton_.SetVisible(state_.twoButtons);
        cancelButton_.SetEnabled(
            state_.twoButtons && state_.cancelEnabled);
        (void)okButton_.Update();
        (void)cancelButton_.Update();

        SetText(okLabel_, state_.okLabel);
        SetText(cancelLabel_, state_.cancelLabel);

        document_->UpdateDocument();

        if (input_ && state_.inputKind != InputKind::None)
            (void)input_->Focus();

        return true;
    }

    [[nodiscard]] bool Update()
    {
        if (!IsLoaded())
            return false;

        bool changed = false;
        changed |= okButton_.Update();
        changed |= cancelButton_.Update();

        if (okButton_.ConsumeClicked())
        {
            QueueAction(ActionType::Ok);
            changed = true;
        }

        if (cancelButton_.ConsumeClicked())
        {
            QueueAction(ActionType::Cancel);
            changed = true;
        }

        return changed;
    }

    [[nodiscard]] std::optional<Action> ConsumeAction()
    {
        return std::exchange(pendingAction_, std::nullopt);
    }

    void ProcessEvent(Rml::Event& event) override
    {
        if (event.GetType() != "keydown")
            return;

        const auto key = static_cast<Rml::Input::KeyIdentifier>(
            event.GetParameter<int>("key_identifier", 0));

        if (key == Rml::Input::KI_RETURN &&
            state_.enterAccepts && state_.okEnabled)
        {
            QueueAction(ActionType::Ok);
            event.StopPropagation();
        }
        else if (key == Rml::Input::KI_ESCAPE &&
                 state_.escapeCancels)
        {
            if (state_.twoButtons && state_.cancelEnabled)
                QueueAction(ActionType::Cancel);
            else
                QueueAction(ActionType::Ok);
            event.StopPropagation();
        }
    }

private:
    void ReadDesign()
    {
        const RmlUiDesign::Values values = LoadDesignValues();

        (void)ParseScalar(values, "RmlMessageBoxPanel-PanelWidth", design_.panelWidth);
        (void)ParseScalar(values, "RmlMessageBoxPanel-PanelHeight", design_.panelHeight);
        (void)ParseScalar(values, "RmlMessageBoxPanel-PanelButtonWidth", design_.buttonWidth);
        (void)ParseScalar(values, "RmlMessageBoxPanel-PanelButtonHeight", design_.buttonHeight);
        (void)ParseScalar(values, "RmlMessageBoxPanel-PanelSingleButtonX", design_.singleButtonX);
        (void)ParseScalar(values, "RmlMessageBoxPanel-PanelOkButtonX", design_.okButtonX);
        (void)ParseScalar(values, "RmlMessageBoxPanel-PanelCancelButtonX", design_.cancelButtonX);
        (void)ParseScalar(values, "RmlMessageBoxPanel-PanelButtonY", design_.buttonY);

        (void)ParseScalar(values, "RmlMessageBoxPanel-PanelCautionWidth", design_.cautionWidth);
        (void)ParseScalar(values, "RmlMessageBoxPanel-PanelCautionHeight", design_.cautionHeight);
        (void)ParseScalar(values, "RmlMessageBoxPanel-PanelCautionButtonWidth", design_.cautionButtonWidth);
        (void)ParseScalar(values, "RmlMessageBoxPanel-PanelCautionButtonHeight", design_.cautionButtonHeight);
        (void)ParseScalar(values, "RmlMessageBoxPanel-PanelCautionSingleButtonX", design_.cautionSingleButtonX);
        (void)ParseScalar(values, "RmlMessageBoxPanel-PanelCautionOkButtonX", design_.cautionOkButtonX);
        (void)ParseScalar(values, "RmlMessageBoxPanel-PanelCautionCancelButtonX", design_.cautionCancelButtonX);
        (void)ParseScalar(values, "RmlMessageBoxPanel-PanelCautionBottomY", design_.cautionBottomY);
        (void)ParseScalar(values, "RmlMessageBoxPanel-PanelCautionMessageHeight", design_.cautionMessageHeight);

        (void)ParseScalar(values, "RmlMessageBoxPanel-PasswordMaxLength", design_.passwordMaxLength);
        (void)ParseScalar(values, "RmlMessageBoxPanel-NumberMaxLength", design_.numberMaxLength);
        (void)ParseScalar(values, "RmlMessageBoxPanel-TextMaxLength", design_.textMaxLength);
    }

    [[nodiscard]] bool BindElements()
    {
        root_ = ElementById(document_, "message-box");
        title_ = ElementById(document_, "message-title");
        message_ = ElementById(document_, "message");
        content_ = ElementById(document_, "message-content");
        input_ = dynamic_cast<Rml::ElementFormControlInput*>(
            ElementById(document_, "message-input"));
        bottom_ = ElementById(document_, "message-bottom");
        okElement_ = ElementById(document_, "ok-button");
        cancelElement_ = ElementById(document_, "cancel-button");
        okLabel_ = ElementById(document_, "ok-label");
        cancelLabel_ = ElementById(document_, "cancel-label");

        if (!root_ || !title_ || !message_ || !content_ ||
            !input_ || !bottom_ || !okElement_ || !cancelElement_ ||
            !okLabel_ || !cancelLabel_)
            return false;

        root_->SetAttribute("tabindex", 0);
        root_->AddEventListener("keydown", this, false);

        okButton_.Bind(okElement_);
        cancelButton_.Bind(cancelElement_);
        return true;
    }

    void ApplyLayout()
    {
        const float width =
            state_.cautionStyle ? design_.cautionWidth : design_.panelWidth;
        const float height =
            state_.cautionStyle ? design_.cautionHeight : design_.panelHeight;

        root_->SetProperty("width", PixelValue(width));
        root_->SetProperty("height", PixelValue(height));
        root_->SetProperty(
            "left",
            PixelValue(
                std::max(
                    0.0f,
                    (static_cast<float>(state_.viewportWidth) - width) * 0.5f)));
        root_->SetProperty(
            "top",
            PixelValue(
                std::max(
                    0.0f,
                    (static_cast<float>(state_.viewportHeight) - height) * 0.5f)));

        const float buttonWidth =
            state_.cautionStyle
                ? design_.cautionButtonWidth
                : design_.buttonWidth;
        const float buttonHeight =
            state_.cautionStyle
                ? design_.cautionButtonHeight
                : design_.buttonHeight;

        const float okX =
            state_.twoButtons
                ? (state_.cautionStyle
                       ? design_.cautionOkButtonX
                       : design_.okButtonX)
                : (state_.cautionStyle
                       ? design_.cautionSingleButtonX
                       : design_.singleButtonX);
        const float cancelX =
            state_.cautionStyle
                ? design_.cautionCancelButtonX
                : design_.cancelButtonX;

        const float buttonY =
            state_.cautionStyle
                ? 10.0f
                : design_.buttonY;

        okElement_->SetProperty("left", PixelValue(okX));
        okElement_->SetProperty("top", PixelValue(buttonY));
        okElement_->SetProperty("width", PixelValue(buttonWidth));
        okElement_->SetProperty("height", PixelValue(buttonHeight));

        cancelElement_->SetProperty("left", PixelValue(cancelX));
        cancelElement_->SetProperty("top", PixelValue(buttonY));
        cancelElement_->SetProperty("width", PixelValue(buttonWidth));
        cancelElement_->SetProperty("height", PixelValue(buttonHeight));

        if (state_.cautionStyle)
            bottom_->SetProperty("top", PixelValue(design_.cautionBottomY));
        else
            bottom_->SetProperty("top", "0px");
    }

    void ApplyMessage()
    {
        content_->SetInnerRML("");

        if (state_.lines.empty())
            return;

        for (const std::string& text : state_.lines)
        {
            Rml::ElementPtr line = document_->CreateElement("div");
            line->SetClass("message-line", true);
            line->AppendChild(document_->CreateTextNode(text));
            content_->AppendChild(std::move(line));
        }
    }

    void ConfigureInput()
    {
        const bool showInput =
            state_.inputKind != InputKind::None &&
            !state_.messageOnly;

        input_->SetProperty("display", showInput ? "block" : "none");
        if (!showInput)
        {
            input_->SetValue("");
            return;
        }

        int maxLength = design_.textMaxLength;
        const char* type = "text";

        switch (state_.inputKind)
        {
        case InputKind::Password:
            maxLength = design_.passwordMaxLength;
            type = "password";
            break;
        case InputKind::Number:
            maxLength = design_.numberMaxLength;
            type = "text";
            input_->SetAttribute("inputmode", "numeric");
            break;
        case InputKind::Text:
            maxLength = design_.textMaxLength;
            break;
        case InputKind::None:
            break;
        }

        input_->SetAttribute("type", type);
        input_->SetAttribute("maxlength", maxLength);
        input_->SetValue(state_.input);
    }

    void QueueAction(ActionType type)
    {
        Action action;
        action.type = type;
        if (input_ && state_.inputKind != InputKind::None)
            action.input = input_->GetValue();
        pendingAction_ = std::move(action);
    }

    RmlDocumentHost host_;
    Rml::ElementDocument* document_ = nullptr;
    bool hostVisible_ = false;

    Rml::Element* root_ = nullptr;
    Rml::Element* title_ = nullptr;
    Rml::Element* message_ = nullptr;
    Rml::Element* content_ = nullptr;
    Rml::ElementFormControlInput* input_ = nullptr;
    Rml::Element* bottom_ = nullptr;
    Rml::Element* okElement_ = nullptr;
    Rml::Element* cancelElement_ = nullptr;
    Rml::Element* okLabel_ = nullptr;
    Rml::Element* cancelLabel_ = nullptr;

    RmlMuButton okButton_;
    RmlMuButton cancelButton_;

    Design design_;
    State state_;
    std::optional<Action> pendingAction_;
};

RmlMessageBoxPanel::RmlMessageBoxPanel()
    : m_impl(std::make_unique<Impl>())
{
}

RmlMessageBoxPanel::~RmlMessageBoxPanel() = default;
RmlMessageBoxPanel::RmlMessageBoxPanel(
    RmlMessageBoxPanel&&) noexcept = default;
RmlMessageBoxPanel& RmlMessageBoxPanel::operator=(
    RmlMessageBoxPanel&&) noexcept = default;

bool RmlMessageBoxPanel::Load(bool show)
{
    return m_impl && m_impl->Load(show);
}
bool RmlMessageBoxPanel::Show()
{
    return m_impl && m_impl->Show();
}
bool RmlMessageBoxPanel::Hide()
{
    return m_impl && m_impl->Hide();
}
void RmlMessageBoxPanel::Release()
{
    if (m_impl) m_impl->Release();
}
bool RmlMessageBoxPanel::IsLoaded() const noexcept
{
    return m_impl && m_impl->IsLoaded();
}
bool RmlMessageBoxPanel::ApplyState(const State& state)
{
    return m_impl && m_impl->ApplyState(state);
}
bool RmlMessageBoxPanel::Update()
{
    return m_impl && m_impl->Update();
}
std::optional<RmlMessageBoxPanel::Action>
RmlMessageBoxPanel::ConsumeAction()
{
    return m_impl ? m_impl->ConsumeAction() : std::nullopt;
}

} // namespace UI::Modern::PC::Common

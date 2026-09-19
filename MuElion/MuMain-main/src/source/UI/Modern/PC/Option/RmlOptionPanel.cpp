#include "stdafx.h"
#include "UI/Modern/PC/Option/RmlOptionPanel.h"

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
#include <RmlUi/Core/Elements/ElementFormControl.h>

#include <algorithm>
#include <array>
#include <charconv>
#include <cstdio>
#include <sstream>
#include <string_view>
#include <utility>

namespace UI::Modern::PC::Option
{

namespace
{
constexpr const char* kDocumentPath =
    "Data/UI/PC/Option/option.rml";

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

Rml::ElementFormControl* FormById(
    Rml::ElementDocument* document, const char* id)
{
    return dynamic_cast<Rml::ElementFormControl*>(
        ElementById(document, id));
}

void SetText(Rml::Element* element, const std::string& text)
{
    if (!element)
        return;
    element->SetInnerRML("");
    if (Rml::ElementDocument* document = element->GetOwnerDocument())
        element->AppendChild(document->CreateTextNode(text));
}

Rml::String PixelValue(float value)
{
    std::array<char, 64> buffer{};
    std::snprintf(
        buffer.data(), buffer.size(), "%.3fpx",
        static_cast<double>(value));
    return Rml::String(buffer.data());
}

bool ParseIndex(const Rml::String& value, int& index)
{
    const std::string_view view(value.data(), value.size());
    if (view.empty())
        return false;

    int parsed = 0;
    const auto [end, error] =
        std::from_chars(view.data(), view.data() + view.size(), parsed);
    if (error != std::errc{} || end != view.data() + view.size())
        return false;

    index = parsed;
    return true;
}
} // namespace

class RmlOptionPanel::Impl final : public Rml::EventListener
{
public:
    struct Design
    {
        float width = 390.0f;
        float height = 580.0f;
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

        const auto values = LoadDesignValues();
        (void)ParseScalar(values, "Option-Width", design_.width);
        (void)ParseScalar(values, "Option-Height", design_.height);

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

        for (Rml::ElementFormControl* select :
             {fontSelect_, languageSelect_, resolutionSelect_})
        {
            if (select)
                select->RemoveEventListener("change", this, false);
        }

        automatic_.Unbind();
        whisper_.Unbind();
        nameDisplay_.Unbind();
        slide_.Unbind();
        render_.Unbind();
        windowed_.Unbind();
        for (auto& button : soundButtons_) button.Unbind();
        for (auto& button : musicButtons_) button.Unbind();
        for (auto& button : effectButtons_) button.Unbind();
        close_.Unbind();

        document_ = nullptr;
        root_ = nullptr;
        title_ = nullptr;
        automaticInput_ = whisperInput_ = nameDisplayInput_ = nullptr;
        slideInput_ = renderInput_ = windowedInput_ = nullptr;
        fontSelect_ = languageSelect_ = resolutionSelect_ = nullptr;
        soundInputs_.fill(nullptr);
        musicInputs_.fill(nullptr);
        effectInputs_.fill(nullptr);
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
        state_.soundVolume = std::clamp(state_.soundVolume, 0, 10);
        state_.musicVolume = std::clamp(state_.musicVolume, 0, 10);
        state_.effectLevel = std::clamp(state_.effectLevel, 0, 4);

        if (!IsLoaded())
            return false;

        const bool visible =
            hostVisible_ && state_.visible &&
            state_.viewportWidth > 0 && state_.viewportHeight > 0;
        root_->SetProperty("display", visible ? "block" : "none");
        if (!visible)
            return true;

        ApplyLayout();
        ApplyLabels();
        ApplyChecks();
        ApplyMeters();
        ApplySelects();

        document_->UpdateDocument();
        return true;
    }

    [[nodiscard]] bool Update()
    {
        if (!IsLoaded())
            return false;

        bool changed = false;

        changed |= PollToggle(
            automatic_, ActionType::AutomaticAttack,
            !state_.automaticAttack);
        changed |= PollToggle(
            whisper_, ActionType::WhisperSound,
            !state_.whisperSound);
        changed |= PollToggle(
            nameDisplay_, ActionType::NameDisplay,
            !state_.nameDisplay);
        changed |= PollToggle(
            slide_, ActionType::SlideHelp,
            !state_.slideHelp);
        changed |= PollToggle(
            render_, ActionType::RenderAllEffects,
            !state_.renderAllEffects);
        changed |= PollToggle(
            windowed_, ActionType::WindowedMode,
            !state_.windowedMode);

        for (int i = 0; i <= 10; ++i)
        {
            changed |= soundButtons_[i].Update();
            if (soundButtons_[i].ConsumeClicked())
            {
                pendingAction_ = Action{ActionType::SoundVolume, i};
                changed = true;
            }

            changed |= musicButtons_[i].Update();
            if (musicButtons_[i].ConsumeClicked())
            {
                pendingAction_ = Action{ActionType::MusicVolume, i};
                changed = true;
            }
        }

        for (int i = 0; i <= 4; ++i)
        {
            changed |= effectButtons_[i].Update();
            if (effectButtons_[i].ConsumeClicked())
            {
                pendingAction_ = Action{ActionType::EffectLevel, i};
                changed = true;
            }
        }

        changed |= close_.Update();
        if (close_.ConsumeClicked())
        {
            pendingAction_ = Action{ActionType::Close, 0};
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
        if (event.GetType() == "keydown")
        {
            const auto key =
                static_cast<Rml::Input::KeyIdentifier>(
                    event.GetParameter<int>("key_identifier", 0));
            if (key == Rml::Input::KI_ESCAPE)
            {
                pendingAction_ = Action{ActionType::Close, 0};
                event.StopPropagation();
            }
            return;
        }

        if (event.GetType() != "change")
            return;

        auto* form = dynamic_cast<Rml::ElementFormControl*>(
            event.GetCurrentElement());
        if (!form)
            return;

        int index = -1;
        if (!ParseIndex(form->GetValue(), index))
            return;

        const Rml::String& id = form->GetId();
        if (id == "font-select")
            pendingAction_ = Action{ActionType::Font, index};
        else if (id == "language-select")
            pendingAction_ = Action{ActionType::Language, index};
        else if (id == "resolution-select")
            pendingAction_ = Action{ActionType::Resolution, index};
    }

private:
    [[nodiscard]] bool PollToggle(
        RmlMuButton& button,
        ActionType type,
        bool newValue)
    {
        bool changed = button.Update();
        if (button.ConsumeClicked())
        {
            pendingAction_ =
                Action{type, newValue ? 1 : 0};
            changed = true;
        }
        return changed;
    }

    [[nodiscard]] bool BindElements()
    {
        root_ = ElementById(document_, "option-panel");
        title_ = ElementById(document_, "title");

        automaticInput_ = ElementById(document_, "automatic-check");
        whisperInput_ = ElementById(document_, "whisper-check");
        nameDisplayInput_ = ElementById(document_, "name-display-check");
        slideInput_ = ElementById(document_, "slide-check");
        renderInput_ = ElementById(document_, "render-check");
        windowedInput_ = ElementById(document_, "windowed-check");

        fontSelect_ = FormById(document_, "font-select");
        languageSelect_ = FormById(document_, "language-select");
        resolutionSelect_ = FormById(document_, "resolution-select");

        if (!root_ || !title_ ||
            !automaticInput_ || !whisperInput_ || !nameDisplayInput_ ||
            !slideInput_ || !renderInput_ || !windowedInput_ ||
            !fontSelect_ || !languageSelect_ || !resolutionSelect_)
            return false;

        automatic_.Bind(automaticInput_);
        whisper_.Bind(whisperInput_);
        nameDisplay_.Bind(nameDisplayInput_);
        slide_.Bind(slideInput_);
        render_.Bind(renderInput_);
        windowed_.Bind(windowedInput_);

        for (int i = 0; i <= 10; ++i)
        {
            soundInputs_[i] =
                ElementById(document_, "sound-" + std::to_string(i));
            musicInputs_[i] =
                ElementById(document_, "music-" + std::to_string(i));
            if (!soundInputs_[i] || !musicInputs_[i])
                return false;
            soundButtons_[i].Bind(soundInputs_[i]);
            musicButtons_[i].Bind(musicInputs_[i]);
        }

        for (int i = 0; i <= 4; ++i)
        {
            effectInputs_[i] =
                ElementById(document_, "effect-" + std::to_string(i));
            if (!effectInputs_[i])
                return false;
            effectButtons_[i].Bind(effectInputs_[i]);
        }

        close_.Bind(ElementById(document_, "close-button"));

        root_->SetAttribute("tabindex", 0);
        root_->AddEventListener("keydown", this, false);
        fontSelect_->AddEventListener("change", this, false);
        languageSelect_->AddEventListener("change", this, false);
        resolutionSelect_->AddEventListener("change", this, false);
        return true;
    }

    void ApplyLayout()
    {
        root_->SetProperty("width", PixelValue(design_.width));
        root_->SetProperty("height", PixelValue(design_.height));
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
                 design_.height) * 0.5f)));
    }

    void ApplyLabels()
    {
        SetText(title_, state_.title);
        SetText(ElementById(document_, "automatic-label"), state_.automaticLabel);
        SetText(ElementById(document_, "whisper-label"), state_.whisperLabel);
        SetText(ElementById(document_, "name-display-label"), state_.nameDisplayLabel);
        SetText(ElementById(document_, "sound-label"), state_.soundLabel);
        SetText(ElementById(document_, "music-label"), state_.musicLabel);
        SetText(ElementById(document_, "slide-label"), state_.slideLabel);
        SetText(ElementById(document_, "effect-label"), state_.effectLabel);
        SetText(ElementById(document_, "render-label"), state_.renderLabel);
        SetText(ElementById(document_, "font-label"), state_.fontLabel);
        SetText(ElementById(document_, "language-label"), state_.languageLabel);
        SetText(ElementById(document_, "resolution-label"), state_.resolutionLabel);
        SetText(ElementById(document_, "windowed-label"), state_.windowedLabel);
        SetText(ElementById(document_, "close-label"), state_.closeLabel);
    }

    void ApplyChecks()
    {
        automaticInput_->SetAttribute("checked", state_.automaticAttack);
        whisperInput_->SetAttribute("checked", state_.whisperSound);
        nameDisplayInput_->SetAttribute("checked", state_.nameDisplay);
        slideInput_->SetAttribute("checked", state_.slideHelp);
        renderInput_->SetAttribute("checked", state_.renderAllEffects);
        windowedInput_->SetAttribute("checked", state_.windowedMode);
    }

    void ApplyMeters()
    {
        for (int i = 0; i <= 10; ++i)
        {
            soundInputs_[i]->SetAttribute(
                "checked", i == state_.soundVolume);
            musicInputs_[i]->SetAttribute(
                "checked", i == state_.musicVolume);
        }

        for (int i = 0; i <= 4; ++i)
            effectInputs_[i]->SetAttribute(
                "checked", i == state_.effectLevel);
    }

    void PopulateSelect(
        Rml::ElementFormControl* select,
        const std::vector<std::string>& values,
        int selected)
    {
        if (!select)
            return;

        select->SetInnerRML("");
        for (std::size_t i = 0; i < values.size(); ++i)
        {
            Rml::ElementPtr option =
                document_->CreateElement("option");
            option->SetAttribute("value", std::to_string(i));
            option->AppendChild(
                document_->CreateTextNode(values[i]));
            select->AppendChild(std::move(option));
        }

        if (!values.empty())
        {
            selected = std::clamp(
                selected, 0, static_cast<int>(values.size() - 1u));
            select->SetValue(std::to_string(selected));
        }
    }

    void ApplySelects()
    {
        PopulateSelect(fontSelect_, state_.fonts, state_.fontIndex);
        PopulateSelect(
            languageSelect_, state_.languages, state_.languageIndex);
        PopulateSelect(
            resolutionSelect_, state_.resolutions, state_.resolutionIndex);
    }

    RmlDocumentHost host_;
    Rml::ElementDocument* document_ = nullptr;
    bool hostVisible_ = false;

    Rml::Element* root_ = nullptr;
    Rml::Element* title_ = nullptr;

    Rml::Element* automaticInput_ = nullptr;
    Rml::Element* whisperInput_ = nullptr;
    Rml::Element* nameDisplayInput_ = nullptr;
    Rml::Element* slideInput_ = nullptr;
    Rml::Element* renderInput_ = nullptr;
    Rml::Element* windowedInput_ = nullptr;

    std::array<Rml::Element*, 11> soundInputs_{};
    std::array<Rml::Element*, 11> musicInputs_{};
    std::array<Rml::Element*, 5> effectInputs_{};

    Rml::ElementFormControl* fontSelect_ = nullptr;
    Rml::ElementFormControl* languageSelect_ = nullptr;
    Rml::ElementFormControl* resolutionSelect_ = nullptr;

    RmlMuButton automatic_;
    RmlMuButton whisper_;
    RmlMuButton nameDisplay_;
    RmlMuButton slide_;
    RmlMuButton render_;
    RmlMuButton windowed_;
    std::array<RmlMuButton, 11> soundButtons_{};
    std::array<RmlMuButton, 11> musicButtons_{};
    std::array<RmlMuButton, 5> effectButtons_{};
    RmlMuButton close_;

    Design design_;
    State state_;
    std::optional<Action> pendingAction_;
};

RmlOptionPanel::RmlOptionPanel()
    : m_impl(std::make_unique<Impl>()) {}
RmlOptionPanel::~RmlOptionPanel() = default;
RmlOptionPanel::RmlOptionPanel(RmlOptionPanel&&) noexcept = default;
RmlOptionPanel& RmlOptionPanel::operator=(
    RmlOptionPanel&&) noexcept = default;

bool RmlOptionPanel::Load(bool show) { return m_impl && m_impl->Load(show); }
bool RmlOptionPanel::Show() { return m_impl && m_impl->Show(); }
bool RmlOptionPanel::Hide() { return m_impl && m_impl->Hide(); }
void RmlOptionPanel::Release() { if (m_impl) m_impl->Release(); }
bool RmlOptionPanel::IsLoaded() const noexcept { return m_impl && m_impl->IsLoaded(); }
bool RmlOptionPanel::ApplyState(const State& state) { return m_impl && m_impl->ApplyState(state); }
bool RmlOptionPanel::Update() { return m_impl && m_impl->Update(); }
std::optional<RmlOptionPanel::Action> RmlOptionPanel::ConsumeAction()
{
    return m_impl ? m_impl->ConsumeAction() : std::nullopt;
}

} // namespace UI::Modern::PC::Option

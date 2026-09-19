#include "stdafx.h"
#include "UI/Modern/PC/Character/RmlPetInfoPanel.h"

#include "UI/Modern/RmlDocumentHost.h"
#include "UI/Modern/RmlMuButton.h"
#include "UI/Modern/RmlMuMovablePanel.h"
#include "UI/Modern/RmlMuScrollBar.h"
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

namespace UI::Modern::PC::Character
{

namespace
{
constexpr const char* kDocumentPath =
    "Data/UI/PC/Character/pet_info.rml";

Rml::String PixelValue(float value)
{
    std::array<char, 64> buffer{};
    std::snprintf(
        buffer.data(), buffer.size(), "%.3fpx",
        static_cast<double>(value));
    return Rml::String(buffer.data());
}

Rml::String PercentValue(float ratio)
{
    std::array<char, 64> buffer{};
    std::snprintf(
        buffer.data(), buffer.size(), "%.3f%%",
        static_cast<double>(
            std::clamp(ratio, 0.0f, 1.0f) * 100.0f));
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
    Rml::ElementDocument* document, const Rml::String& id)
{
    return document ? document->GetElementById(id) : nullptr;
}

void SetText(Rml::Element* element, const std::string& text)
{
    if (element)
        element->SetInnerRML(text);
}

} // namespace

class RmlPetInfoPanel::Impl
{
public:
    static constexpr std::size_t kVisibleSkillRows = 4;

    struct Design
    {
        float panelWidth = 327.0f;
        float panelHeight = 639.0f;
        float referenceWidth = 640.0f;
        float referenceHeight = 480.0f;
        float gfxStageWidth = 1024.0f;
        float initialX = 339.0f;
        float initialY = 55.0f;
        std::array<float, 2> tabX{27.0f, 136.0f};
        float tabY = 85.0f;
        float tabWidth = 110.0f;
        float tabHeight = 31.0f;
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
        for (auto& button : tabButtons_)
            button.Unbind();
        closeButton_.Unbind();
        scroll_.Unbind();
        mover_.Unbind();

        document_ = nullptr;
        root_ = nullptr;
        drag_ = nullptr;
        closeElement_ = nullptr;
        title_ = nullptr;
        content_ = nullptr;
        missing_ = nullptr;
        commands_ = nullptr;
        progressFill_ = nullptr;
        leadershipLabel_ = leadershipValue_ = commandLabel_ = nullptr;
        fieldLabels_.fill(nullptr);
        fieldValues_.fill(nullptr);
        tabElements_.fill(nullptr);
        tabLabels_.fill(nullptr);
        skillRows_.fill(nullptr);
        skillSlots_.fill(nullptr);
        skillLabels_.fill(nullptr);

        skillScrollStart_ = 0;
        positionInitialized_ = false;
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
        state_.selectedTab =
            std::min<std::size_t>(state_.selectedTab, 1u);

        if (!IsLoaded())
            return false;

        const bool visible =
            state_.visible &&
            state_.viewportWidth > 0 &&
            state_.viewportHeight > 0;
        root_->SetProperty("display", visible ? "block" : "none");
        if (!visible)
            return true;

        ApplyLayout();
        SyncState();
        document_->UpdateDocument();
        return true;
    }

    [[nodiscard]] bool Update()
    {
        if (!IsLoaded())
            return false;

        bool changed = false;

        for (std::size_t i = 0; i < tabButtons_.size(); ++i)
        {
            changed |= tabButtons_[i].Update();
            if (tabButtons_[i].ConsumeClicked())
            {
                state_.selectedTab = i;
                pendingAction_ =
                    Action{ActionType::SelectTab, i};
                SyncTabs();
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

        const std::size_t maximum = SkillMaximumStart();
        (void)scroll_.SetState(
            skillScrollStart_,
            maximum,
            kVisibleSkillRows,
            kVisibleSkillRows,
            state_.commandsVisible && maximum > 0,
            true);

        if (const auto request =
                scroll_.ConsumeRequestedPosition())
        {
            skillScrollStart_ =
                std::min(*request, maximum);
            SyncSkills();
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
            values, "RmlPetInfoPanel-PanelWidth",
            design_.panelWidth);
        (void)ParseScalar(
            values, "RmlPetInfoPanel-PanelHeight",
            design_.panelHeight);
        (void)ParseScalar(
            values, "RmlPetInfoPanel-ReferenceWidth",
            design_.referenceWidth);
        (void)ParseScalar(
            values, "RmlPetInfoPanel-ReferenceHeight",
            design_.referenceHeight);
        (void)ParseScalar(
            values, "RmlPetInfoPanel-GfxStageWidth",
            design_.gfxStageWidth);
        (void)ParseScalar(
            values, "RmlPetInfoPanel-InitialX",
            design_.initialX);
        (void)ParseScalar(
            values, "RmlPetInfoPanel-InitialY",
            design_.initialY);
        (void)ParseScalar(
            values, "RmlPetInfoPanel-TabY",
            design_.tabY);
        (void)ParseScalar(
            values, "RmlPetInfoPanel-TabWidth",
            design_.tabWidth);
        (void)ParseScalar(
            values, "RmlPetInfoPanel-TabHeight",
            design_.tabHeight);

        if (const auto it =
                values.find("RmlPetInfoPanel-TabX");
            it != values.end())
        {
            std::istringstream stream(it->second);
            (void)(stream >> design_.tabX[0] >> design_.tabX[1]);
        }
    }

    [[nodiscard]] bool BindElements()
    {
        root_ = RequiredElement(document_, "pet-info");
        drag_ = RequiredElement(document_, "pet-info-drag");
        closeElement_ =
            RequiredElement(document_, "pet-info-close");
        title_ = RequiredElement(document_, "pet-info-title");
        content_ =
            RequiredElement(document_, "pet-info-content");
        missing_ =
            RequiredElement(document_, "pet-info-missing");
        commands_ =
            RequiredElement(document_, "pet-info-commands");
        progressFill_ =
            RequiredElement(document_, "pet-info-progress-fill");
        leadershipLabel_ =
            RequiredElement(document_, "pet-info-leadership-label");
        leadershipValue_ =
            RequiredElement(document_, "pet-info-leadership");
        commandLabel_ =
            RequiredElement(document_, "pet-info-command-label");

        if (!root_ || !drag_ || !closeElement_ || !title_ ||
            !content_ || !missing_ || !commands_ ||
            !progressFill_ || !leadershipLabel_ ||
            !leadershipValue_ || !commandLabel_)
            return false;

        for (std::size_t i = 0; i < 2; ++i)
        {
            const std::string suffix = std::to_string(i);
            tabElements_[i] =
                RequiredElement(document_, "pet-info-tab-" + suffix);
            tabLabels_[i] =
                RequiredElement(
                    document_,
                    "pet-info-tab-label-" + suffix);
            if (!tabElements_[i] || !tabLabels_[i])
                return false;
            tabButtons_[i].Bind(tabElements_[i]);
        }

        for (std::size_t i = 0; i < fieldLabels_.size(); ++i)
        {
            const std::string suffix = std::to_string(i);
            fieldLabels_[i] =
                RequiredElement(
                    document_, "pet-info-label-" + suffix);
            fieldValues_[i] =
                RequiredElement(
                    document_, "pet-info-value-" + suffix);
            if (!fieldLabels_[i] || !fieldValues_[i])
                return false;
        }

        for (std::size_t i = 0; i < skillRows_.size(); ++i)
        {
            const std::string suffix = std::to_string(i);
            skillRows_[i] =
                RequiredElement(
                    document_, "pet-info-skill-row-" + suffix);
            skillSlots_[i] =
                RequiredElement(
                    document_, "pet-info-skill-slot-" + suffix);
            skillLabels_[i] =
                RequiredElement(
                    document_, "pet-info-skill-label-" + suffix);
            if (!skillRows_[i] || !skillSlots_[i] || !skillLabels_[i])
                return false;
        }

        if (!scroll_.Bind(
                RequiredElement(document_, "pet-info-scroll")))
            return false;

        closeButton_.Bind(closeElement_);
        mover_.Bind(root_, drag_);
        return true;
    }

    void ApplyLayout()
    {
        const float availableWidth =
            static_cast<float>(state_.viewportWidth);
        const float availableHeight =
            static_cast<float>(state_.viewportHeight);

        const float designRight =
            std::max(1.0f, design_.initialX + design_.panelWidth);
        const float designBottom =
            std::max(1.0f, design_.initialY + design_.panelHeight);

        const float scale = std::clamp(
            std::min(
                availableWidth / designRight,
                availableHeight / designBottom),
            0.25f, 1.0f);

        root_->SetProperty("width", PixelValue(design_.panelWidth));
        root_->SetProperty("height", PixelValue(design_.panelHeight));
        root_->SetProperty("transform-origin", "0 0");
        root_->SetProperty("transform", ScaleValue(scale));

        for (std::size_t i = 0; i < tabElements_.size(); ++i)
        {
            tabElements_[i]->SetProperty(
                "left", PixelValue(design_.tabX[i]));
            tabElements_[i]->SetProperty(
                "top", PixelValue(design_.tabY));
            tabElements_[i]->SetProperty(
                "width", PixelValue(design_.tabWidth));
            tabElements_[i]->SetProperty(
                "height", PixelValue(design_.tabHeight));
        }

        mover_.SetMetrics(
            availableWidth / scale,
            availableHeight / scale,
            design_.panelWidth,
            design_.panelHeight,
            0.0f,
            0.0f);

        if (!positionInitialized_)
        {
            mover_.SetPosition(
                design_.initialX, design_.initialY);
            positionInitialized_ = true;
        }
    }

    void SyncState()
    {
        SetText(title_, state_.title);
        SetText(missing_, state_.missingText);

        missing_->SetProperty(
            "display", state_.missing ? "block" : "none");
        content_->SetProperty(
            "display", state_.missing ? "none" : "block");
        commands_->SetProperty(
            "display",
            (!state_.missing && state_.commandsVisible)
                ? "block" : "none");

        for (std::size_t i = 0; i < state_.fields.size(); ++i)
        {
            const Field& field = state_.fields[i];
            SetText(fieldLabels_[i], field.label);
            SetText(fieldValues_[i], field.value);
            fieldLabels_[i]->SetProperty(
                "display", field.visible ? "block" : "none");
            fieldValues_[i]->SetProperty(
                "display", field.visible ? "block" : "none");
        }

        progressFill_->SetProperty(
            "width", PercentValue(state_.progressRatio));

        SetText(leadershipLabel_, state_.leadershipLabel);
        SetText(leadershipValue_, state_.leadershipValue);
        SetText(commandLabel_, state_.commandLabel);

        SyncTabs();
        SyncSkills();
    }

    void SyncTabs()
    {
        for (std::size_t i = 0; i < tabElements_.size(); ++i)
        {
            SetText(tabLabels_[i], state_.tabLabels[i]);
            tabElements_[i]->SetClass(
                "selected", state_.selectedTab == i);
        }
    }

    [[nodiscard]] std::size_t SkillMaximumStart() const noexcept
    {
        return state_.skills.size() > kVisibleSkillRows
            ? state_.skills.size() - kVisibleSkillRows
            : 0u;
    }

    void SyncSkills()
    {
        const std::size_t maximum = SkillMaximumStart();
        skillScrollStart_ =
            std::min(skillScrollStart_, maximum);

        for (std::size_t row = 0; row < skillRows_.size(); ++row)
        {
            const std::size_t index =
                skillScrollStart_ + row;
            const bool visible =
                state_.commandsVisible &&
                index < state_.skills.size();

            skillRows_[row]->SetProperty(
                "display", visible ? "block" : "none");
            if (!visible)
                continue;

            const Skill& skill = state_.skills[index];
            skillRows_[row]->SetClass(
                "disabled", !skill.enabled);
            skillSlots_[row]->SetAttribute(
                "data-skill-id", skill.skillId);
            SetText(skillLabels_[row], skill.label);
        }

        (void)scroll_.SetState(
            skillScrollStart_,
            maximum,
            kVisibleSkillRows,
            kVisibleSkillRows,
            state_.commandsVisible && maximum > 0,
            true);
    }

    RmlDocumentHost host_;
    Rml::ElementDocument* document_ = nullptr;

    Rml::Element* root_ = nullptr;
    Rml::Element* drag_ = nullptr;
    Rml::Element* closeElement_ = nullptr;
    Rml::Element* title_ = nullptr;
    Rml::Element* content_ = nullptr;
    Rml::Element* missing_ = nullptr;
    Rml::Element* commands_ = nullptr;
    Rml::Element* progressFill_ = nullptr;
    Rml::Element* leadershipLabel_ = nullptr;
    Rml::Element* leadershipValue_ = nullptr;
    Rml::Element* commandLabel_ = nullptr;

    std::array<Rml::Element*, 2> tabElements_{};
    std::array<Rml::Element*, 2> tabLabels_{};
    std::array<Rml::Element*, 5> fieldLabels_{};
    std::array<Rml::Element*, 5> fieldValues_{};
    std::array<Rml::Element*, 4> skillRows_{};
    std::array<Rml::Element*, 4> skillSlots_{};
    std::array<Rml::Element*, 4> skillLabels_{};

    std::array<RmlMuButton, 2> tabButtons_{};
    RmlMuButton closeButton_;
    RmlMuScrollBar scroll_;
    RmlMuMovablePanel mover_;

    Design design_;
    State state_;
    std::size_t skillScrollStart_ = 0;
    bool positionInitialized_ = false;
    std::optional<Action> pendingAction_;
};

RmlPetInfoPanel::RmlPetInfoPanel()
    : m_impl(std::make_unique<Impl>())
{
}

RmlPetInfoPanel::~RmlPetInfoPanel() = default;
RmlPetInfoPanel::RmlPetInfoPanel(
    RmlPetInfoPanel&&) noexcept = default;
RmlPetInfoPanel& RmlPetInfoPanel::operator=(
    RmlPetInfoPanel&&) noexcept = default;

bool RmlPetInfoPanel::Load(bool show)
{
    return m_impl && m_impl->Load(show);
}

bool RmlPetInfoPanel::Show()
{
    return m_impl && m_impl->Show();
}

bool RmlPetInfoPanel::Hide()
{
    return m_impl && m_impl->Hide();
}

void RmlPetInfoPanel::Release()
{
    if (m_impl)
        m_impl->Release();
}

bool RmlPetInfoPanel::IsLoaded() const noexcept
{
    return m_impl && m_impl->IsLoaded();
}

bool RmlPetInfoPanel::ApplyState(const State& state)
{
    return m_impl && m_impl->ApplyState(state);
}

bool RmlPetInfoPanel::Update()
{
    return m_impl && m_impl->Update();
}

std::optional<RmlPetInfoPanel::Action>
RmlPetInfoPanel::ConsumeAction()
{
    return m_impl
        ? m_impl->ConsumeAction()
        : std::nullopt;
}

} // namespace UI::Modern::PC::Character

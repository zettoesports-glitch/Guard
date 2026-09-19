#include "stdafx.h"
#include "UI/Modern/PC/Character/RmlCharacterFramePanel.h"

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

namespace UI::Modern::PC::Character
{

namespace
{
constexpr const char* kDocumentPath =
    "Data/UI/PC/Character/character_frame.rml";

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

class RmlCharacterFramePanel::Impl
{
public:
    struct Design
    {
        float panelWidth = 327.0f;
        float panelHeight = 639.0f;
        float referenceWidth = 640.0f;
        float referenceHeight = 480.0f;
        float gfxStageWidth = 1024.0f;
        float initialX = 639.0f;
        float initialY = 40.0f;
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
        mover_.Unbind();
        closeButton_.Unbind();
        petButton_.Unbind();
        masterButton_.Unbind();
        for (auto& button : statButtons_)
            button.Unbind();

        document_ = nullptr;
        root_ = nullptr;
        drag_ = nullptr;
        title_ = nullptr;
        levelLabel_ = level_ = nullptr;
        classLabel_ = class_ = nullptr;
        serverLabel_ = server_ = nullptr;
        experience_ = nullptr;
        pointLabel_ = points_ = nullptr;
        petLabel_ = masterLabel_ = nullptr;
        closeElement_ = petElement_ = masterElement_ = nullptr;

        statLabels_.fill(nullptr);
        statValues_.fill(nullptr);
        statDetails_.fill(nullptr);
        statButtonElements_.fill(nullptr);
        statSections_.fill(nullptr);
        statFields_.fill(nullptr);

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

        ApplyLayout();

        SetText(title_, state_.title);
        SetText(levelLabel_, state_.levelLabel);
        SetText(level_, state_.level);
        SetText(classLabel_, state_.classLabel);
        SetText(class_, state_.characterClass);
        SetText(serverLabel_, state_.serverLabel);
        SetText(server_, state_.server);
        SetText(experience_, state_.experience);
        SetText(pointLabel_, state_.pointsLabel);
        SetText(points_, state_.points);
        SetText(petLabel_, state_.petLabel);
        SetText(masterLabel_, state_.masterLabel);

        for (std::size_t i = 0; i < state_.stats.size(); ++i)
        {
            const Stat& stat = state_.stats[i];
            const bool shown = stat.visible;

            SetText(statLabels_[i], stat.label);
            SetText(statValues_[i], stat.value);
            SetText(statDetails_[i], stat.detail);

            if (statLabels_[i])
                statLabels_[i]->SetProperty(
                    "display", shown ? "block" : "none");
            if (statValues_[i])
                statValues_[i]->SetProperty(
                    "display", shown ? "block" : "none");
            if (statDetails_[i])
                statDetails_[i]->SetProperty(
                    "display", shown ? "block" : "none");
            if (statSections_[i])
                statSections_[i]->SetProperty(
                    "display", shown ? "block" : "none");
            if (statFields_[i])
                statFields_[i]->SetProperty(
                    "display", shown ? "block" : "none");

            statButtons_[i].SetVisible(shown && stat.canIncrease);
            statButtons_[i].SetEnabled(shown && stat.canIncrease);
            (void)statButtons_[i].Update();
        }

        petButton_.SetEnabled(state_.petEnabled);
        masterButton_.SetEnabled(state_.masterEnabled);
        (void)petButton_.Update();
        (void)masterButton_.Update();
        (void)closeButton_.Update();

        document_->UpdateDocument();
        return true;
    }

    [[nodiscard]] bool Update()
    {
        if (!IsLoaded())
            return false;

        bool changed = false;

        changed |= closeButton_.Update();
        changed |= petButton_.Update();
        changed |= masterButton_.Update();

        for (std::size_t i = 0; i < statButtons_.size(); ++i)
        {
            changed |= statButtons_[i].Update();
            if (statButtons_[i].ConsumeClicked())
            {
                pendingAction_ = Action{
                    ActionType::IncreaseStat, i};
                changed = true;
            }
        }

        if (petButton_.ConsumeClicked())
        {
            pendingAction_ = Action{ActionType::OpenPet, 0};
            changed = true;
        }

        if (masterButton_.ConsumeClicked())
        {
            pendingAction_ = Action{ActionType::OpenMaster, 0};
            changed = true;
        }

        if (closeButton_.ConsumeClicked())
        {
            pendingAction_ = Action{ActionType::Close, 0};
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
            values, "RmlCharacterFramePanel-PanelWidth",
            design_.panelWidth);
        (void)ParseScalar(
            values, "RmlCharacterFramePanel-PanelHeight",
            design_.panelHeight);
        (void)ParseScalar(
            values, "RmlCharacterFramePanel-ReferenceWidth",
            design_.referenceWidth);
        (void)ParseScalar(
            values, "RmlCharacterFramePanel-ReferenceHeight",
            design_.referenceHeight);
        (void)ParseScalar(
            values, "RmlCharacterFramePanel-GfxStageWidth",
            design_.gfxStageWidth);
        (void)ParseScalar(
            values, "RmlCharacterFramePanel-InitialX",
            design_.initialX);
        (void)ParseScalar(
            values, "RmlCharacterFramePanel-InitialY",
            design_.initialY);
    }

    [[nodiscard]] bool BindElements()
    {
        root_ = RequiredElement(document_, "character-frame");
        drag_ = RequiredElement(document_, "character-drag");
        title_ = RequiredElement(document_, "character-title");
        levelLabel_ =
            RequiredElement(document_, "character-level-label");
        level_ = RequiredElement(document_, "character-level");
        classLabel_ =
            RequiredElement(document_, "character-class-label");
        class_ = RequiredElement(document_, "character-class");
        serverLabel_ =
            RequiredElement(document_, "character-server-label");
        server_ = RequiredElement(document_, "character-server");
        experience_ =
            RequiredElement(document_, "character-experience");
        pointLabel_ =
            RequiredElement(document_, "character-point-label");
        points_ = RequiredElement(document_, "character-points");
        petElement_ = RequiredElement(document_, "character-pet");
        petLabel_ =
            RequiredElement(document_, "character-pet-label");
        masterElement_ =
            RequiredElement(document_, "character-master");
        masterLabel_ =
            RequiredElement(document_, "character-master-label");
        closeElement_ =
            RequiredElement(document_, "character-close");

        if (!root_ || !drag_ || !title_ || !levelLabel_ || !level_ ||
            !classLabel_ || !class_ || !serverLabel_ || !server_ ||
            !experience_ || !pointLabel_ || !points_ || !petElement_ ||
            !petLabel_ || !masterElement_ || !masterLabel_ ||
            !closeElement_)
            return false;

        static constexpr std::array<const char*, 5> suffixes{
            "0", "1", "2", "3", "4"};

        for (std::size_t i = 0; i < suffixes.size(); ++i)
        {
            const std::string suffix = suffixes[i];
            statLabels_[i] = RequiredElement(
                document_, "character-stat-label-" + suffix);
            statValues_[i] = RequiredElement(
                document_, "character-stat-value-" + suffix);
            statDetails_[i] = RequiredElement(
                document_, "character-detail-" + suffix);
            statButtonElements_[i] = RequiredElement(
                document_, "character-stat-button-" + suffix);
            statFields_[i] = RequiredElement(
                document_, "character-field-stat-" + suffix);

            static constexpr std::array<const char*, 5> sectionIds{
                "character-section-strength",
                "character-section-dexterity",
                "character-section-vitality",
                "character-section-energy",
                "character-section-charisma"};
            statSections_[i] =
                RequiredElement(document_, sectionIds[i]);

            if (!statLabels_[i] || !statValues_[i] ||
                !statButtonElements_[i] || !statFields_[i] ||
                !statSections_[i])
                return false;

            statButtons_[i].Bind(statButtonElements_[i]);
        }

        closeButton_.Bind(closeElement_);
        petButton_.Bind(petElement_);
        masterButton_.Bind(masterElement_);
        mover_.Bind(root_, drag_);
        return true;
    }

    void ApplyLayout()
    {
        if (!root_)
            return;

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

        mover_.SetMetrics(
            availableWidth / scale,
            availableHeight / scale,
            design_.panelWidth,
            design_.panelHeight,
            0.0f,
            0.0f);

        const Rml::Vector2f current = mover_.GetPosition();
        const bool hasMoved =
            current.x != 0.0f || current.y != 0.0f;
        if (!hasMoved)
            mover_.SetPosition(design_.initialX, design_.initialY);
    }

    RmlDocumentHost host_;
    Rml::ElementDocument* document_ = nullptr;

    Rml::Element* root_ = nullptr;
    Rml::Element* drag_ = nullptr;
    Rml::Element* title_ = nullptr;
    Rml::Element* levelLabel_ = nullptr;
    Rml::Element* level_ = nullptr;
    Rml::Element* classLabel_ = nullptr;
    Rml::Element* class_ = nullptr;
    Rml::Element* serverLabel_ = nullptr;
    Rml::Element* server_ = nullptr;
    Rml::Element* experience_ = nullptr;
    Rml::Element* pointLabel_ = nullptr;
    Rml::Element* points_ = nullptr;

    std::array<Rml::Element*, 5> statLabels_{};
    std::array<Rml::Element*, 5> statValues_{};
    std::array<Rml::Element*, 5> statDetails_{};
    std::array<Rml::Element*, 5> statButtonElements_{};
    std::array<Rml::Element*, 5> statSections_{};
    std::array<Rml::Element*, 5> statFields_{};

    Rml::Element* petElement_ = nullptr;
    Rml::Element* petLabel_ = nullptr;
    Rml::Element* masterElement_ = nullptr;
    Rml::Element* masterLabel_ = nullptr;
    Rml::Element* closeElement_ = nullptr;

    std::array<RmlMuButton, 5> statButtons_{};
    RmlMuButton petButton_;
    RmlMuButton masterButton_;
    RmlMuButton closeButton_;
    RmlMuMovablePanel mover_;

    Design design_;
    State state_;
    std::optional<Action> pendingAction_;
};

RmlCharacterFramePanel::RmlCharacterFramePanel()
    : m_impl(std::make_unique<Impl>())
{
}

RmlCharacterFramePanel::~RmlCharacterFramePanel() = default;
RmlCharacterFramePanel::RmlCharacterFramePanel(
    RmlCharacterFramePanel&&) noexcept = default;
RmlCharacterFramePanel& RmlCharacterFramePanel::operator=(
    RmlCharacterFramePanel&&) noexcept = default;

bool RmlCharacterFramePanel::Load(bool show)
{
    return m_impl && m_impl->Load(show);
}

bool RmlCharacterFramePanel::Show()
{
    return m_impl && m_impl->Show();
}

bool RmlCharacterFramePanel::Hide()
{
    return m_impl && m_impl->Hide();
}

void RmlCharacterFramePanel::Release()
{
    if (m_impl)
        m_impl->Release();
}

bool RmlCharacterFramePanel::IsLoaded() const noexcept
{
    return m_impl && m_impl->IsLoaded();
}

bool RmlCharacterFramePanel::ApplyState(const State& state)
{
    return m_impl && m_impl->ApplyState(state);
}

bool RmlCharacterFramePanel::Update()
{
    return m_impl && m_impl->Update();
}

std::optional<RmlCharacterFramePanel::Action>
RmlCharacterFramePanel::ConsumeAction()
{
    return m_impl
        ? m_impl->ConsumeAction()
        : std::nullopt;
}

} // namespace UI::Modern::PC::Character

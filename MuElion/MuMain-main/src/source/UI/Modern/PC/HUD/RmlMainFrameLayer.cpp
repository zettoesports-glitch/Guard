#include "stdafx.h"
#include "UI/Modern/PC/HUD/RmlMainFrameLayer.h"

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
#include <cmath>
#include <cstdio>
#include <initializer_list>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>

namespace UI::Modern::PC::HUD
{

namespace
{
constexpr const char* kDocumentPath = "Data/UI/PC/HUD/main_frame.rml";
constexpr const char* kSkillListDocumentPath =
    "Data/UI/PC/HUD/skill_list_icons.rml";
constexpr const char* kSkillIconInnerRml =
    "<div class=\"skill-sheet\"/><div class=\"skill-cooldown\"/>";

Rml::String PixelValue(float value)
{
    std::array<char, 64> buffer{};
    std::snprintf(
        buffer.data(), buffer.size(), "%.3fpx",
        static_cast<double>(value));
    return Rml::String(buffer.data());
}

Rml::String PercentValue(float value)
{
    std::array<char, 64> buffer{};
    std::snprintf(
        buffer.data(), buffer.size(), "%.3f%%",
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

float Ratio(
    const RmlMainFrameLayer::Gauge& gauge) noexcept
{
    if (gauge.maximum <= 0)
        return 0.0f;

    const double value = std::clamp(
        static_cast<double>(gauge.current) /
            static_cast<double>(gauge.maximum),
        0.0, 1.0);
    return static_cast<float>(value);
}

Rml::String GaugeText(
    const RmlMainFrameLayer::Gauge& gauge)
{
    return std::to_string(gauge.current) + " / " +
           std::to_string(gauge.maximum);
}

void SetText(Rml::Element* element, const Rml::String& value)
{
    if (element)
        element->SetInnerRML(value);
}

void SetExclusiveClasses(
    Rml::Element* element,
    const char* selected,
    std::initializer_list<const char*> names)
{
    if (!element)
        return;

    for (const char* name : names)
        element->SetClass(name, std::string_view(name) == selected);
}

} // namespace

class RmlMainFrameLayer::Impl
{
public:
    struct Design
    {
        float resizeStageHeight = 700.0f;
        float smallStageScale = 0.95f;
        float artWidth = 921.0f;
        float artHeight = 106.0f;
        float hotSelectionX = 475.0f;
        float hotSelectionStep = 32.0f;
        float marbleHeight = 89.0f;
        float legacyReferenceWidth = 640.0f;
        float legacyReferenceHeight = 480.0f;

        // Observable retained CNewUISkillList arrangement.
        std::size_t skillListCenteredCount = 14;
        std::size_t skillListFirstRowCount = 18;
        float skillListLeftRunStart = -8.0f;
        float skillListSecondRowStart = -11.0f;
        float skillListSlotWidth = 32.0f;
        float skillListSlotHeight = 38.0f;
        float skillListIconOffset = 6.0f;
    };

    Impl()
        : host_(kDocumentPath),
          skillListHost_(kSkillListDocumentPath)
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
        if (!BindElements() || !LoadSkillListDocument())
        {
            Release();
            return false;
        }

        loadedVisible_ = show;
        if (show)
            state_.visible = true;
        stateDirty_ = true;
        return ApplyState(state_);
    }

    [[nodiscard]] bool Show()
    {
        if (!host_.IsLoaded() && !Load(false))
            return false;

        loadedVisible_ = true;
        state_.visible = true;
        stateDirty_ = true;
        const bool shown = host_.Show();
        if (shown)
            (void)ApplyState(state_);
        return shown;
    }

    [[nodiscard]] bool Hide()
    {
        loadedVisible_ = false;
        state_.visible = false;
        stateDirty_ = true;
        return host_.Hide();
    }

    void Release()
    {
        ClearSkillListElements();
        skillListRoot_ = nullptr;
        skillListDocument_ = nullptr;
        skillListHost_.Release();

        UnbindElements();
        document_ = nullptr;
        host_.Release();
        pendingAction_.reset();
        stateDirty_ = false;
    }

    [[nodiscard]] bool IsLoaded() const noexcept
    {
        return host_.IsLoaded() && document_ != nullptr;
    }

    [[nodiscard]] bool ApplyState(const State& state)
    {
        state_ = state;
        stateDirty_ = false;

        if (!IsLoaded())
            return false;

        SyncVisibilityAndLayout();
        SyncGauges();
        SyncExperience();
        SyncSkillPage();
        SyncSkillIcons();
        SyncSkillList();
        SyncButtons();
        document_->UpdateDocument();
        return true;
    }

    [[nodiscard]] bool Update()
    {
        if (!IsLoaded())
            return false;

        bool changed = false;
        for (RmlMuButton* button : AllButtons())
            changed |= button->Update();

        auto consume = [&](RmlMuButton& button, Action action) {
            if (button.ConsumeClicked())
            {
                pendingAction_ = action;
                changed = true;
            }
        };

        consume(shopButton_, Action::Shop);
        consume(characterButton_, Action::Character);
        consume(inventoryButton_, Action::Inventory);
        consume(questButton_, Action::Quest);
        consume(communityButton_, Action::Community);
        consume(systemButton_, Action::System);
        consume(skillPageButton_, Action::ToggleSkillPage);

        if (stateDirty_)
        {
            (void)ApplyState(state_);
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
            values, "RmlMainFrameLayer-ResizeStageHeight",
            design_.resizeStageHeight);
        (void)ParseScalar(
            values, "RmlMainFrameLayer-SmallStageScale",
            design_.smallStageScale);
        (void)ParseScalar(
            values, "RmlMainFrameLayer-ArtWidth",
            design_.artWidth);
        (void)ParseScalar(
            values, "RmlMainFrameLayer-ArtHeight",
            design_.artHeight);
        (void)ParseScalar(
            values, "RmlMainFrameLayer-HotSelectionX",
            design_.hotSelectionX);
        (void)ParseScalar(
            values, "RmlMainFrameLayer-HotSelectionStep",
            design_.hotSelectionStep);
        (void)ParseScalar(
            values, "RmlMainFrameLayer-MarbleHeight",
            design_.marbleHeight);
        (void)ParseScalar(
            values, "RmlMainFrameLayer-LegacyReferenceWidth",
            design_.legacyReferenceWidth);
        (void)ParseScalar(
            values, "RmlMainFrameLayer-LegacyReferenceHeight",
            design_.legacyReferenceHeight);

        (void)ParseScalar(
            values, "NewUIMainFrameWindow-SkillListCenteredCount",
            design_.skillListCenteredCount);
        (void)ParseScalar(
            values, "NewUIMainFrameWindow-SkillListFirstRowCount",
            design_.skillListFirstRowCount);
        (void)ParseScalar(
            values, "NewUIMainFrameWindow-SkillListLeftRunStart",
            design_.skillListLeftRunStart);
        (void)ParseScalar(
            values, "NewUIMainFrameWindow-SkillListSecondRowStart",
            design_.skillListSecondRowStart);
        (void)ParseScalar(
            values, "NewUIMainFrameWindow-SkillListSlotWidth",
            design_.skillListSlotWidth);
        (void)ParseScalar(
            values, "NewUIMainFrameWindow-SkillListSlotHeight",
            design_.skillListSlotHeight);
        (void)ParseScalar(
            values, "NewUIMainFrameWindow-SkillListIconOffset",
            design_.skillListIconOffset);
    }

    [[nodiscard]] bool BindElements()
    {
        frame_ = RequiredElement(document_, "main-frame");
        shell_ = RequiredElement(document_, "main-shell");
        hpBackground_ = RequiredElement(document_, "main-hp-background");
        hpClip_ = RequiredElement(document_, "main-hp-clip");
        hp_ = RequiredElement(document_, "main-hp");
        mpBackground_ = RequiredElement(document_, "main-mp-background");
        mpClip_ = RequiredElement(document_, "main-mp-clip");
        mp_ = RequiredElement(document_, "main-mp");
        sdClip_ = RequiredElement(document_, "main-sd-clip");
        sd_ = RequiredElement(document_, "main-sd");
        agClip_ = RequiredElement(document_, "main-ag-clip");
        ag_ = RequiredElement(document_, "main-ag");
        expClip_ = RequiredElement(document_, "main-exp-clip");
        exp_ = RequiredElement(document_, "main-exp");
        expFrame_ = RequiredElement(document_, "main-exp-frame");
        hpText_ = RequiredElement(document_, "main-hp-text");
        mpText_ = RequiredElement(document_, "main-mp-text");
        sdText_ = RequiredElement(document_, "main-sd-text");
        agText_ = RequiredElement(document_, "main-ag-text");
        expCurrent_ = RequiredElement(document_, "main-exp-current");
        expNext_ = RequiredElement(document_, "main-exp-next");
        expSlash_ = RequiredElement(document_, "main-exp-slash");
        expPage_ = RequiredElement(document_, "main-exp-page");
        currentSelection_ =
            RequiredElement(document_, "main-current-selection");
        hotSelection_ =
            RequiredElement(document_, "main-hot-selection");
        skillPageLabels_ =
            RequiredElement(document_, "main-skill-page-labels");
        skillPageToggle_ =
            RequiredElement(document_, "main-skill-page-toggle");

        if (!frame_ || !shell_ || !hpBackground_ || !hpClip_ || !hp_ ||
            !mpBackground_ || !mpClip_ || !mp_ || !sdClip_ || !sd_ ||
            !agClip_ || !ag_ || !expClip_ || !exp_ || !expFrame_ ||
            !hpText_ || !mpText_ || !sdText_ || !agText_ ||
            !expCurrent_ || !expNext_ || !expSlash_ || !expPage_ ||
            !currentSelection_ || !hotSelection_ || !skillPageLabels_ ||
            !skillPageToggle_)
            return false;

        for (std::size_t index = 0; index < skillElements_.size(); ++index)
        {
            const std::string id =
                "main-skill-icon-" + std::to_string(index);
            skillElements_[index] =
                RequiredElement(document_, id.c_str());
            if (!skillElements_[index])
                return false;
        }

        auto bind = [&](RmlMuButton& button, const char* id) {
            Rml::Element* element = RequiredElement(document_, id);
            if (!element)
                return false;
            button.Bind(element);
            return true;
        };

        return bind(skillPageButton_, "main-skill-page-toggle") &&
               bind(shopButton_, "main-button-shop") &&
               bind(characterButton_, "main-button-character") &&
               bind(inventoryButton_, "main-button-inventory") &&
               bind(questButton_, "main-button-quest") &&
               bind(communityButton_, "main-button-community") &&
               bind(systemButton_, "main-button-system");
    }

    [[nodiscard]] bool LoadSkillListDocument()
    {
        if (!skillListHost_.Load(false))
            return false;

        skillListDocument_ = skillListHost_.GetDocument();
        if (!skillListDocument_)
            return false;

        skillListRoot_ =
            RequiredElement(skillListDocument_, "skill-list-icons");
        return skillListRoot_ != nullptr;
    }

    void ClearSkillListElements()
    {
        skillListElements_.clear();
        if (skillListRoot_)
            skillListRoot_->SetInnerRML("");
    }

    [[nodiscard]] bool EnsureSkillListElementCount(std::size_t count)
    {
        if (!skillListDocument_ || !skillListRoot_)
            return false;

        if (skillListElements_.size() == count)
            return true;

        ClearSkillListElements();
        skillListElements_.reserve(count);

        // Main-x64-Debug function at 0x1406743f0:
        // CreateElement("div"), id "skill-list-icon-N", class
        // "mu-skill-icon", exact two-child InnerRML, append to root.
        for (std::size_t index = 0; index < count; ++index)
        {
            Rml::ElementPtr element =
                skillListDocument_->CreateElement("div");
            if (!element)
                return false;

            element->SetId(
                Rml::String("skill-list-icon-") +
                std::to_string(index));
            element->SetClass("mu-skill-icon", true);
            element->SetInnerRML(kSkillIconInnerRml);

            Rml::Element* raw =
                skillListRoot_->AppendChild(std::move(element));
            if (!raw)
                return false;

            skillListElements_.push_back(raw);
        }

        return true;
    }

    void UnbindElements()
    {
        for (RmlMuButton* button : AllButtons())
            button->Unbind();

        frame_ = nullptr;
        shell_ = nullptr;
        hpBackground_ = nullptr;
        hpClip_ = nullptr;
        hp_ = nullptr;
        mpBackground_ = nullptr;
        mpClip_ = nullptr;
        mp_ = nullptr;
        sdClip_ = nullptr;
        sd_ = nullptr;
        agClip_ = nullptr;
        ag_ = nullptr;
        expClip_ = nullptr;
        exp_ = nullptr;
        expFrame_ = nullptr;
        hpText_ = nullptr;
        mpText_ = nullptr;
        sdText_ = nullptr;
        agText_ = nullptr;
        expCurrent_ = nullptr;
        expNext_ = nullptr;
        expSlash_ = nullptr;
        expPage_ = nullptr;
        currentSelection_ = nullptr;
        hotSelection_ = nullptr;
        skillPageLabels_ = nullptr;
        skillPageToggle_ = nullptr;
        skillElements_.fill(nullptr);
    }

    [[nodiscard]] std::array<RmlMuButton*, 7> AllButtons()
    {
        return {
            &skillPageButton_,
            &shopButton_,
            &characterButton_,
            &inventoryButton_,
            &questButton_,
            &communityButton_,
            &systemButton_,
        };
    }

    void SyncVisibilityAndLayout()
    {
        if (!frame_)
            return;

        const bool visible =
            state_.visible &&
            state_.viewportWidth > 0 &&
            state_.viewportHeight > 0;

        frame_->SetProperty("display", visible ? "block" : "none");
        if (!visible)
            return;

        // Main-x64-Debug writes transform using the exact format
        // "scale(%.6f)". The recovered design threshold selects the authored
        // small-stage scale for short viewports.
        const float scale =
            static_cast<float>(state_.viewportHeight) <
                    design_.resizeStageHeight
                ? design_.smallStageScale
                : 1.0f;

        const float scaledWidth = design_.artWidth * scale;
        const float scaledHeight = design_.artHeight * scale;
        const float left = std::max(
            0.0f,
            (static_cast<float>(state_.viewportWidth) - scaledWidth) * 0.5f);
        const float top = std::max(
            0.0f,
            static_cast<float>(state_.viewportHeight) - scaledHeight);

        frame_->SetProperty("left", PixelValue(left));
        frame_->SetProperty("top", PixelValue(top));
        frame_->SetProperty("transform", ScaleValue(scale));
    }

    void SyncGauges()
    {
        const float hpRatio = Ratio(state_.hp);
        const float mpRatio = Ratio(state_.mp);
        const float sdRatio = Ratio(state_.sd);
        const float agRatio = Ratio(state_.ag);

        if (hp_)
        {
            hp_->SetProperty(
                "height", PixelValue(design_.marbleHeight * hpRatio));
            hp_->SetClass("poisoned", state_.poisoned);
            hp_->SetClass("normal", !state_.poisoned);
        }

        if (mp_)
            mp_->SetProperty(
                "height", PixelValue(design_.marbleHeight * mpRatio));

        if (sd_)
            sd_->SetProperty("width", PercentValue(sdRatio * 100.0f));

        if (ag_)
            ag_->SetProperty("width", PercentValue(agRatio * 100.0f));

        SetText(hpText_, GaugeText(state_.hp));
        SetText(mpText_, GaugeText(state_.mp));
        SetText(sdText_, GaugeText(state_.sd));
        SetText(agText_, GaugeText(state_.ag));
    }

    void SyncExperience()
    {
        const float expRatio = Ratio(state_.experience);
        if (exp_)
            exp_->SetProperty(
                "width", PercentValue(expRatio * 100.0f));

        const char* style = "normal";
        switch (state_.experienceStyle)
        {
        case ExperienceStyle::Master:
            style = "master";
            break;
        case ExperienceStyle::Fourth:
            style = "fourth";
            break;
        case ExperienceStyle::Normal:
        default:
            style = "normal";
            break;
        }

        SetExclusiveClasses(
            exp_, style, {"normal", "master", "fourth"});

        SetText(
            expCurrent_,
            std::to_string(state_.experience.current));
        SetText(
            expNext_,
            std::to_string(state_.experience.maximum));
        SetText(expSlash_, "/");
        SetText(expPage_, std::to_string(state_.experiencePage));
    }

    void SyncSkillPage()
    {
        if (skillPageLabels_)
        {
            skillPageLabels_->SetClass(
                "page-one", !state_.secondSkillPage);
            skillPageLabels_->SetClass(
                "page-two", state_.secondSkillPage);

            static constexpr std::array<const char*, 5> kPageOne{
                "1", "2", "3", "4", "5"};
            static constexpr std::array<const char*, 5> kPageTwo{
                "6", "7", "8", "9", "0"};
            const auto& labels =
                state_.secondSkillPage ? kPageTwo : kPageOne;

            const int count = std::min(
                skillPageLabels_->GetNumChildren(),
                static_cast<int>(labels.size()));
            for (int index = 0; index < count; ++index)
            {
                if (Rml::Element* child =
                        skillPageLabels_->GetChild(index))
                    child->SetInnerRML(labels[
                        static_cast<std::size_t>(index)]);
            }
        }

        if (skillPageToggle_)
        {
            skillPageToggle_->SetClass(
                "to-first", state_.secondSkillPage);
            skillPageToggle_->SetClass(
                "to-second", !state_.secondSkillPage);
        }

        skillPageButton_.SetEnabled(state_.skillPageEnabled);
        (void)skillPageButton_.Update();

        if (currentSelection_)
        {
            currentSelection_->SetProperty(
                "display",
                state_.showCurrentSelection ? "block" : "none");
        }

        if (hotSelection_)
        {
            const bool showHot =
                state_.hotSelectionSlot >= 0 &&
                state_.hotSelectionSlot < 5;

            hotSelection_->SetProperty(
                "display", showHot ? "block" : "none");

            if (showHot)
            {
                hotSelection_->SetProperty(
                    "left",
                    PixelValue(
                        design_.hotSelectionX +
                        design_.hotSelectionStep *
                            static_cast<float>(
                                state_.hotSelectionSlot)));
            }
        }
    }

    void SyncSkillIcons()
    {
        for (std::size_t index = 0;
             index < skillElements_.size();
             ++index)
        {
            Rml::Element* element = skillElements_[index];
            if (!element)
                continue;

            const SkillIcon& skill = state_.skills[index];
            element->SetProperty(
                "display", skill.visible ? "block" : "none");
            element->SetClass("disabled", !skill.enabled);
            element->SetAttribute("data-skill-id", skill.skillId);

            if (element->GetNumChildren() >= 2)
            {
                Rml::Element* cooldown = element->GetChild(1);
                if (cooldown)
                {
                    const float ratio =
                        std::clamp(skill.cooldownRatio, 0.0f, 1.0f);
                    cooldown->SetProperty(
                        "height",
                        PercentValue(ratio * 100.0f));
                }
            }
        }
    }

    void SyncSkillList()
    {
        if (!skillListDocument_ || !skillListRoot_)
            return;

        const bool visible =
            state_.visible &&
            state_.skillListVisible &&
            state_.viewportWidth > 0 &&
            state_.viewportHeight > 0;

        skillListRoot_->SetProperty(
            "display", visible ? "block" : "none");

        if (!visible)
        {
            (void)skillListHost_.Hide();
            return;
        }

        if (!EnsureSkillListElementCount(
                state_.skillListSkills.size()))
            return;

        (void)skillListHost_.Show();

        // Legacy CNewUISkillList starts at x=385, y=390 in the 640x480
        // reference space. Its placement formula is retained exactly here:
        // alternating centered 14, four left-run entries, then second row.
        constexpr float kLegacyOriginX = 385.0f;
        constexpr float kLegacyOriginY = 390.0f;

        const float scaleX =
            state_.viewportWidth > 0 && design_.legacyReferenceWidth > 0.0f
                ? static_cast<float>(state_.viewportWidth) /
                    design_.legacyReferenceWidth
                : 1.0f;
        const float scaleY =
            state_.viewportHeight > 0 && design_.legacyReferenceHeight > 0.0f
                ? static_cast<float>(state_.viewportHeight) /
                    design_.legacyReferenceHeight
                : 1.0f;

        for (std::size_t index = 0;
             index < skillListElements_.size();
             ++index)
        {
            Rml::Element* element = skillListElements_[index];
            if (!element)
                continue;

            float x = kLegacyOriginX;
            float y = kLegacyOriginY;

            if (index >= design_.skillListFirstRowCount)
                y -= design_.skillListSlotHeight;

            if (index < design_.skillListCenteredCount)
            {
                const std::size_t quotient = index / 2u;
                if ((index % 2u) == 0u)
                {
                    x = kLegacyOriginX +
                        static_cast<float>(quotient) *
                            design_.skillListSlotWidth;
                }
                else
                {
                    x = kLegacyOriginX -
                        static_cast<float>(quotient + 1u) *
                            design_.skillListSlotWidth;
                }
            }
            else if (index < design_.skillListFirstRowCount)
            {
                x = kLegacyOriginX +
                    design_.skillListLeftRunStart *
                        design_.skillListSlotWidth -
                    static_cast<float>(
                        index - design_.skillListCenteredCount) *
                        design_.skillListSlotWidth;
            }
            else
            {
                x = kLegacyOriginX +
                    design_.skillListSecondRowStart *
                        design_.skillListSlotWidth +
                    static_cast<float>(
                        index - design_.skillListFirstRowCount + 1u) *
                        design_.skillListSlotWidth;
            }

            const SkillIcon& skill = state_.skillListSkills[index];
            element->SetProperty(
                "left", PixelValue(x * scaleX));
            element->SetProperty(
                "top", PixelValue(y * scaleY));
            element->SetProperty(
                "width",
                PixelValue(design_.skillListSlotWidth * scaleX));
            element->SetProperty(
                "height",
                PixelValue(design_.skillListSlotHeight * scaleY));
            element->SetProperty(
                "display", skill.visible ? "block" : "none");
            element->SetClass("disabled", !skill.enabled);
            element->SetAttribute("data-skill-id", skill.skillId);

            if (element->GetNumChildren() >= 2)
            {
                if (Rml::Element* cooldown = element->GetChild(1))
                {
                    const float ratio =
                        std::clamp(skill.cooldownRatio, 0.0f, 1.0f);
                    cooldown->SetProperty(
                        "height", PercentValue(ratio * 100.0f));
                }
            }
        }

        skillListDocument_->UpdateDocument();
    }

    void SyncButtons()
    {
        for (RmlMuButton* button : AllButtons())
            (void)button->Update();
    }

    RmlDocumentHost host_;
    RmlDocumentHost skillListHost_;
    Rml::ElementDocument* document_ = nullptr;
    Rml::ElementDocument* skillListDocument_ = nullptr;
    Rml::Element* skillListRoot_ = nullptr;
    std::vector<Rml::Element*> skillListElements_;

    Rml::Element* frame_ = nullptr;
    Rml::Element* shell_ = nullptr;
    Rml::Element* hpBackground_ = nullptr;
    Rml::Element* hpClip_ = nullptr;
    Rml::Element* hp_ = nullptr;
    Rml::Element* mpBackground_ = nullptr;
    Rml::Element* mpClip_ = nullptr;
    Rml::Element* mp_ = nullptr;
    Rml::Element* sdClip_ = nullptr;
    Rml::Element* sd_ = nullptr;
    Rml::Element* agClip_ = nullptr;
    Rml::Element* ag_ = nullptr;
    Rml::Element* expClip_ = nullptr;
    Rml::Element* exp_ = nullptr;
    Rml::Element* expFrame_ = nullptr;
    Rml::Element* hpText_ = nullptr;
    Rml::Element* mpText_ = nullptr;
    Rml::Element* sdText_ = nullptr;
    Rml::Element* agText_ = nullptr;
    Rml::Element* expCurrent_ = nullptr;
    Rml::Element* expNext_ = nullptr;
    Rml::Element* expSlash_ = nullptr;
    Rml::Element* expPage_ = nullptr;
    Rml::Element* currentSelection_ = nullptr;
    Rml::Element* hotSelection_ = nullptr;
    Rml::Element* skillPageLabels_ = nullptr;
    Rml::Element* skillPageToggle_ = nullptr;
    std::array<Rml::Element*, 6> skillElements_{};

    RmlMuButton skillPageButton_;
    RmlMuButton shopButton_;
    RmlMuButton characterButton_;
    RmlMuButton inventoryButton_;
    RmlMuButton questButton_;
    RmlMuButton communityButton_;
    RmlMuButton systemButton_;

    Design design_;
    State state_;
    bool loadedVisible_ = false;
    bool stateDirty_ = false;
    std::optional<Action> pendingAction_;
};

RmlMainFrameLayer::RmlMainFrameLayer()
    : m_impl(std::make_unique<Impl>())
{
}

RmlMainFrameLayer::~RmlMainFrameLayer() = default;
RmlMainFrameLayer::RmlMainFrameLayer(RmlMainFrameLayer&&) noexcept = default;
RmlMainFrameLayer& RmlMainFrameLayer::operator=(
    RmlMainFrameLayer&&) noexcept = default;

bool RmlMainFrameLayer::Load(bool show)
{
    return m_impl && m_impl->Load(show);
}

bool RmlMainFrameLayer::Show()
{
    return m_impl && m_impl->Show();
}

bool RmlMainFrameLayer::Hide()
{
    return m_impl && m_impl->Hide();
}

void RmlMainFrameLayer::Release()
{
    if (m_impl)
        m_impl->Release();
}

bool RmlMainFrameLayer::IsLoaded() const noexcept
{
    return m_impl && m_impl->IsLoaded();
}

bool RmlMainFrameLayer::ApplyState(const State& state)
{
    return m_impl && m_impl->ApplyState(state);
}

bool RmlMainFrameLayer::Update()
{
    return m_impl && m_impl->Update();
}

std::optional<RmlMainFrameLayer::Action>
RmlMainFrameLayer::ConsumeAction()
{
    return m_impl
        ? m_impl->ConsumeAction()
        : std::nullopt;
}

} // namespace UI::Modern::PC::HUD

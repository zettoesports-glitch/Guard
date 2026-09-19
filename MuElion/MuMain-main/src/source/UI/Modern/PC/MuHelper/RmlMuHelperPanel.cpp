#include "stdafx.h"
#include "UI/Modern/PC/MuHelper/RmlMuHelperPanel.h"

#include "UI/Modern/RmlDocumentHost.h"
#include "UI/Modern/RmlMuButton.h"
#include "UI/Modern/RmlMuMovablePanel.h"
#include "UI/Modern/RmlMuScrollBar.h"
#include "UI/Modern/RmlMuSlot.h"
#include "UI/Modern/RmlUiDesign.h"

#include <RmlUi/Core/Core.h>
#include <RmlUi/Core/Element.h>
#include <RmlUi/Core/ElementDocument.h>
#include <RmlUi/Core/FileInterface.h>
#include <RmlUi/Core/StreamMemory.h>
#include <RmlUi/Core/Elements/ElementFormControlInput.h>

#include <algorithm>
#include <array>
#include <cstdio>
#include <sstream>
#include <utility>

namespace UI::Modern::PC::MuHelper
{

namespace
{
constexpr const char* kDocumentPath =
    "Data/UI/PC/MuHelper/mu_helper.rml";

constexpr std::array<const char*, 21> kOptionIds{
    "fallback-basic",
    "long-counter",
    "return-position",
    "use-potion",
    "support-party",
    "auto-heal",
    "drain-life",
    "buff-duration",
    "combo",
    "use-dark-raven",
    "repair-item",
    "pick-all",
    "pick-selected",
    "pick-jewel",
    "pick-ancient",
    "pick-zen",
    "pick-excellent",
    "pick-extra",
    "accept-friend",
    "accept-guild",
    "self-defense",
};

constexpr std::array<RmlMuHelperPanel::Option, 21> kOptions{
    RmlMuHelperPanel::Option::FallbackBasic,
    RmlMuHelperPanel::Option::LongCounter,
    RmlMuHelperPanel::Option::ReturnPosition,
    RmlMuHelperPanel::Option::UsePotion,
    RmlMuHelperPanel::Option::SupportParty,
    RmlMuHelperPanel::Option::AutoHeal,
    RmlMuHelperPanel::Option::DrainLife,
    RmlMuHelperPanel::Option::BuffDuration,
    RmlMuHelperPanel::Option::UseCombo,
    RmlMuHelperPanel::Option::UseDarkRaven,
    RmlMuHelperPanel::Option::RepairItem,
    RmlMuHelperPanel::Option::PickAll,
    RmlMuHelperPanel::Option::PickSelected,
    RmlMuHelperPanel::Option::PickJewel,
    RmlMuHelperPanel::Option::PickAncient,
    RmlMuHelperPanel::Option::PickZen,
    RmlMuHelperPanel::Option::PickExcellent,
    RmlMuHelperPanel::Option::PickExtra,
    RmlMuHelperPanel::Option::AcceptFriend,
    RmlMuHelperPanel::Option::AcceptGuild,
    RmlMuHelperPanel::Option::SelfDefense,
};

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
    if (!files->LoadFile(kDocumentPath, contents) ||
        contents.empty())
        return {};

    Rml::StreamMemory stream(
        reinterpret_cast<const Rml::byte*>(contents.data()),
        contents.size());
    return RmlUiDesign::Parse(&stream);
}

Rml::Element* RequiredElement(
    Rml::ElementDocument* document,
    const Rml::String& id)
{
    return document ? document->GetElementById(id) : nullptr;
}

Rml::ElementFormControlInput* InputElement(
    Rml::ElementDocument* document,
    const char* id)
{
    return dynamic_cast<Rml::ElementFormControlInput*>(
        RequiredElement(document, id));
}

int ParseInputInt(
    Rml::ElementFormControlInput* input,
    int fallback,
    int minimum,
    int maximum)
{
    if (!input)
        return fallback;

    std::istringstream stream(input->GetValue());
    int value = fallback;
    if (!(stream >> value))
        value = fallback;

    return std::clamp(value, minimum, maximum);
}

} // namespace

class RmlMuHelperPanel::Impl
{
public:
    struct Design
    {
        float width = 327.0f;
        float height = 639.0f;
        float sidePanelWidth = 306.0f;
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
        resetButton_.Unbind();
        saveButton_.Unbind();
        itemAddButton_.Unbind();
        itemDeleteButton_.Unbind();
        potionConfigButton_.Unbind();
        partyConfigButton_.Unbind();
        skillConfigButtons_[0].Unbind();
        skillConfigButtons_[1].Unbind();

        for (auto& button : tabButtons_)
            button.Unbind();
        for (auto& button : huntRangeButtons_)
            button.Unbind();
        for (auto& button : obtainRangeButtons_)
            button.Unbind();
        for (auto& button : optionButtons_)
            button.Unbind();
        for (auto& button : itemRowButtons_)
            button.Unbind();
        for (auto& slot : assignedSlots_)
            slot.Unbind();
        for (auto& slot : availableSlots_)
            slot.Unbind();

        itemScroll_.Unbind();

        root_ = nullptr;
        drag_ = nullptr;
        pageHunt_ = nullptr;
        pageItem_ = nullptr;
        pageOther_ = nullptr;
        itemList_ = nullptr;
        skillPicker_ = nullptr;
        returnSeconds_ = nullptr;
        manualYieldSeconds_ = nullptr;
        itemName_ = nullptr;
        itemRowElements_.fill(nullptr);
        assignedSlotElements_.fill(nullptr);
        availableSlotElements_.fill(nullptr);
        optionElements_.fill(nullptr);

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
        state_.huntRange = std::clamp(state_.huntRange, 1, 8);
        state_.obtainingRange =
            std::clamp(state_.obtainingRange, 1, 8);

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
        SyncTabs();
        SyncRanges();
        SyncOptions();
        SyncInputs();
        SyncSkills();
        SyncExtraItems(true);

        document_->UpdateDocument();
        return true;
    }

    [[nodiscard]] bool Update()
    {
        if (!IsLoaded())
            return false;

        bool changed = false;

        changed |= closeButton_.Update();
        changed |= resetButton_.Update();
        changed |= saveButton_.Update();
        changed |= itemAddButton_.Update();
        changed |= itemDeleteButton_.Update();
        changed |= potionConfigButton_.Update();
        changed |= partyConfigButton_.Update();
        changed |= skillConfigButtons_[0].Update();
        changed |= skillConfigButtons_[1].Update();

        for (auto& button : tabButtons_)
            changed |= button.Update();
        for (auto& button : huntRangeButtons_)
            changed |= button.Update();
        for (auto& button : obtainRangeButtons_)
            changed |= button.Update();
        for (auto& button : optionButtons_)
            changed |= button.Update();
        for (auto& button : itemRowButtons_)
            changed |= button.Update();
        for (auto& slot : assignedSlots_)
            changed |= slot.Update();
        for (auto& slot : availableSlots_)
            changed |= slot.Update();

        for (std::size_t i = 0; i < tabButtons_.size(); ++i)
        {
            if (!tabButtons_[i].ConsumeClicked())
                continue;

            state_.tab = static_cast<Tab>(i);
            pendingAction_ = Action{
                ActionType::SelectTab,
                Option::FallbackBasic,
                i,
                static_cast<int>(i),
                false,
                {}};
            SyncTabs();
            changed = true;
        }

        for (std::size_t i = 0; i < huntRangeButtons_.size(); ++i)
        {
            if (!huntRangeButtons_[i].ConsumeClicked())
                continue;

            state_.huntRange = static_cast<int>(i + 1u);
            pendingAction_ = Action{
                ActionType::SetHuntRange,
                Option::FallbackBasic,
                i,
                state_.huntRange,
                false,
                {}};
            SyncRanges();
            changed = true;
        }

        for (std::size_t i = 0; i < obtainRangeButtons_.size(); ++i)
        {
            if (!obtainRangeButtons_[i].ConsumeClicked())
                continue;

            state_.obtainingRange = static_cast<int>(i + 1u);
            pendingAction_ = Action{
                ActionType::SetObtainingRange,
                Option::FallbackBasic,
                i,
                state_.obtainingRange,
                false,
                {}};
            SyncRanges();
            changed = true;
        }

        for (std::size_t i = 0; i < optionButtons_.size(); ++i)
        {
            if (!optionButtons_[i].ConsumeClicked())
                continue;

            const Option option = kOptions[i];
            const bool value = !GetOption(option);
            SetOption(option, value);
            pendingAction_ = Action{
                ActionType::ToggleOption,
                option,
                i,
                0,
                value,
                {}};
            SyncOptions();
            changed = true;
        }

        if (closeButton_.ConsumeClicked())
        {
            pendingAction_ = Action{ActionType::Close};
            changed = true;
        }

        if (resetButton_.ConsumeClicked())
        {
            pendingAction_ = Action{ActionType::Reset};
            changed = true;
        }

        if (saveButton_.ConsumeClicked())
        {
            state_.maxSecondsAway =
                ParseInputInt(returnSeconds_, state_.maxSecondsAway, 0, 9999);
            state_.manualYieldSeconds =
                ParseInputInt(
                    manualYieldSeconds_,
                    state_.manualYieldSeconds,
                    0,
                    999);

            pendingAction_ = Action{ActionType::Save};
            changed = true;
        }

        if (potionConfigButton_.ConsumeClicked())
        {
            pendingAction_ =
                Action{ActionType::ConfigurePotion};
            changed = true;
        }

        for (std::size_t i = 0; i < skillConfigButtons_.size(); ++i)
        {
            if (skillConfigButtons_[i].ConsumeClicked())
            {
                pendingAction_ = Action{
                    ActionType::ConfigureSkill,
                    Option::FallbackBasic,
                    i + 1u,
                    static_cast<int>(i + 1u),
                    false,
                    {}};
                changed = true;
            }
        }

        if (partyConfigButton_.ConsumeClicked())
        {
            pendingAction_ =
                Action{ActionType::ConfigureParty};
            changed = true;
        }

        for (std::size_t i = 0; i < assignedSlots_.size(); ++i)
        {
            if (assignedSlots_[i].ConsumeClicked())
            {
                selectedSkillSlot_ = i;
                pendingAction_ = Action{
                    ActionType::SelectAssignedSkill,
                    Option::FallbackBasic,
                    i,
                    state_.assignedSkills[i],
                    false,
                    {}};
                SyncSkills();
                changed = true;
            }

            if (assignedSlots_[i].ConsumeSecondaryClicked())
            {
                pendingAction_ = Action{
                    ActionType::RemoveAssignedSkill,
                    Option::FallbackBasic,
                    i,
                    state_.assignedSkills[i],
                    false,
                    {}};
                changed = true;
            }
        }

        const std::size_t availableCount = std::min(
            availableSlots_.size(), state_.availableSkills.size());
        for (std::size_t i = 0; i < availableCount; ++i)
        {
            if (availableSlots_[i].ConsumeClicked())
            {
                pendingAction_ = Action{
                    ActionType::ChooseAvailableSkill,
                    Option::FallbackBasic,
                    selectedSkillSlot_,
                    state_.availableSkills[i],
                    false,
                    {}};
                changed = true;
            }
        }

        if (itemAddButton_.ConsumeClicked() && itemName_)
        {
            const std::string value = itemName_->GetValue();
            if (!value.empty())
            {
                pendingAction_ = Action{
                    ActionType::AddExtraItem,
                    Option::FallbackBasic,
                    0,
                    0,
                    false,
                    value};
                itemName_->SetValue("");
                changed = true;
            }
        }

        for (std::size_t i = 0; i < itemRowButtons_.size(); ++i)
        {
            if (!itemRowButtons_[i].ConsumeClicked())
                continue;

            const std::size_t absolute = itemScrollStart_ + i;
            if (absolute < state_.extraItems.size())
            {
                selectedExtraItem_ = absolute;
                SyncExtraItems(false);
                changed = true;
            }
        }

        if (itemDeleteButton_.ConsumeClicked() &&
            selectedExtraItem_ &&
            *selectedExtraItem_ < state_.extraItems.size())
        {
            pendingAction_ = Action{
                ActionType::DeleteExtraItem,
                Option::FallbackBasic,
                *selectedExtraItem_,
                0,
                false,
                state_.extraItems[*selectedExtraItem_]};
            changed = true;
        }

        const std::size_t maximum =
            state_.extraItems.size() > itemRowButtons_.size()
                ? state_.extraItems.size() - itemRowButtons_.size()
                : 0u;
        (void)itemScroll_.SetState(
            itemScrollStart_,
            maximum,
            itemRowButtons_.size(),
            itemRowButtons_.size(),
            maximum > 0,
            true);
        if (const auto request = itemScroll_.ConsumeRequestedPosition())
        {
            itemScrollStart_ = std::min(*request, maximum);
            SyncExtraItems(false);
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
        const auto values = LoadDesignValues();
        (void)ParseScalar(values, "MuHelper-Width", design_.width);
        (void)ParseScalar(values, "MuHelper-Height", design_.height);
        (void)ParseScalar(
            values, "MuHelper-SidePanelWidth",
            design_.sidePanelWidth);
    }

    [[nodiscard]] bool BindElements()
    {
        root_ = RequiredElement(document_, "mu-helper");
        drag_ = RequiredElement(document_, "helper-drag");
        pageHunt_ = RequiredElement(document_, "page-hunt");
        pageItem_ = RequiredElement(document_, "page-item");
        pageOther_ = RequiredElement(document_, "page-other");
        skillPicker_ = RequiredElement(document_, "skill-picker");
        itemList_ = RequiredElement(document_, "item-list");

        returnSeconds_ = InputElement(document_, "return-seconds");
        manualYieldSeconds_ =
            InputElement(document_, "manual-yield-seconds");
        itemName_ = InputElement(document_, "item-name");

        if (!root_ || !drag_ || !pageHunt_ || !pageItem_ ||
            !pageOther_ || !skillPicker_ || !itemList_ ||
            !returnSeconds_ || !manualYieldSeconds_ || !itemName_)
            return false;

        auto bindButton = [&](RmlMuButton& button, const char* id) {
            Rml::Element* element = RequiredElement(document_, id);
            if (!element)
                return false;
            button.Bind(element);
            return true;
        };

        if (!bindButton(closeButton_, "helper-close") ||
            !bindButton(resetButton_, "helper-reset") ||
            !bindButton(saveButton_, "helper-save") ||
            !bindButton(itemAddButton_, "item-add") ||
            !bindButton(itemDeleteButton_, "item-delete") ||
            !bindButton(potionConfigButton_, "potion-settings") ||
            !bindButton(partyConfigButton_, "party-settings") ||
            !bindButton(skillConfigButtons_[0], "skill-1-settings") ||
            !bindButton(skillConfigButtons_[1], "skill-2-settings"))
            return false;

        static constexpr std::array<const char*, 3> tabIds{
            "tab-hunt", "tab-item", "tab-other"};
        for (std::size_t i = 0; i < tabIds.size(); ++i)
            if (!bindButton(tabButtons_[i], tabIds[i]))
                return false;

        for (std::size_t i = 0; i < huntRangeButtons_.size(); ++i)
        {
            const std::string hunt =
                "hunt-range-" + std::to_string(i + 1u);
            const std::string obtain =
                "obtain-range-" + std::to_string(i + 1u);

            if (!bindButton(huntRangeButtons_[i], hunt.c_str()) ||
                !bindButton(obtainRangeButtons_[i], obtain.c_str()))
                return false;
        }

        for (std::size_t i = 0; i < optionButtons_.size(); ++i)
        {
            optionElements_[i] =
                RequiredElement(document_, kOptionIds[i]);
            if (!optionElements_[i])
                return false;
            optionButtons_[i].Bind(optionElements_[i]);
        }

        for (std::size_t i = 0; i < assignedSlots_.size(); ++i)
        {
            assignedSlotElements_[i] = RequiredElement(
                document_,
                "assigned-skill-" + std::to_string(i));
            if (!assignedSlotElements_[i])
                return false;
            assignedSlots_[i].Bind(assignedSlotElements_[i]);
        }

        for (std::size_t i = 0; i < availableSlots_.size(); ++i)
        {
            availableSlotElements_[i] = RequiredElement(
                document_,
                "available-skill-" + std::to_string(i));
            if (!availableSlotElements_[i])
                return false;
            availableSlots_[i].Bind(availableSlotElements_[i]);
        }

        for (std::size_t i = 0; i < itemRowButtons_.size(); ++i)
        {
            itemRowElements_[i] = RequiredElement(
                document_,
                "item-row-" + std::to_string(i));
            if (!itemRowElements_[i])
                return false;
            itemRowButtons_[i].Bind(itemRowElements_[i]);
        }

        if (!itemScroll_.Bind(RequiredElement(document_, "item-scroll")))
            return false;

        returnSeconds_->SetAttribute("maxlength", 4);
        manualYieldSeconds_->SetAttribute("maxlength", 3);
        itemName_->SetAttribute("maxlength", 32);

        mover_.Bind(root_, drag_);
        return true;
    }

    void ApplyLayout()
    {
        const float viewportWidth =
            static_cast<float>(state_.viewportWidth);
        const float viewportHeight =
            static_cast<float>(state_.viewportHeight);

        const float scale = std::clamp(
            std::min(
                viewportWidth / std::max(1.0f, design_.width),
                viewportHeight / std::max(1.0f, design_.height)),
            0.25f, 1.0f);

        root_->SetProperty("width", PixelValue(design_.width));
        root_->SetProperty("height", PixelValue(design_.height));
        root_->SetProperty("transform-origin", "0 0");
        root_->SetProperty("transform", ScaleValue(scale));

        mover_.SetMetrics(
            viewportWidth / scale,
            viewportHeight / scale,
            design_.width,
            design_.height,
            0.0f,
            0.0f);

        const Rml::Vector2f current = mover_.GetPosition();
        if (current.x == 0.0f && current.y == 0.0f)
        {
            mover_.SetPosition(
                std::max(0.0f, viewportWidth / scale - design_.width - 20.0f),
                20.0f);
        }
    }

    void SyncTabs()
    {
        pageHunt_->SetProperty(
            "display", state_.tab == Tab::Hunt ? "block" : "none");
        pageItem_->SetProperty(
            "display", state_.tab == Tab::Item ? "block" : "none");
        pageOther_->SetProperty(
            "display", state_.tab == Tab::Other ? "block" : "none");

        for (std::size_t i = 0; i < tabButtons_.size(); ++i)
        {
            if (Rml::Element* element = RequiredElement(
                    document_,
                    std::array<const char*, 3>{
                        "tab-hunt", "tab-item", "tab-other"}[i]))
            {
                element->SetClass(
                    "selected", static_cast<std::size_t>(state_.tab) == i);
            }
        }
    }

    void SyncRanges()
    {
        for (std::size_t i = 0; i < huntRangeButtons_.size(); ++i)
        {
            if (Rml::Element* element = RequiredElement(
                    document_,
                    "hunt-range-" + std::to_string(i + 1u)))
                element->SetClass(
                    "selected", state_.huntRange == static_cast<int>(i + 1u));

            if (Rml::Element* element = RequiredElement(
                    document_,
                    "obtain-range-" + std::to_string(i + 1u)))
                element->SetClass(
                    "selected",
                    state_.obtainingRange == static_cast<int>(i + 1u));
        }
    }

    void SyncOptions()
    {
        for (std::size_t i = 0; i < optionElements_.size(); ++i)
            optionElements_[i]->SetClass(
                "checked", GetOption(kOptions[i]));
    }

    void SyncInputs()
    {
        returnSeconds_->SetValue(std::to_string(state_.maxSecondsAway));
        manualYieldSeconds_->SetValue(
            std::to_string(state_.manualYieldSeconds));
    }

    void SyncSkills()
    {
        for (std::size_t i = 0; i < assignedSlots_.size(); ++i)
        {
            const int skill = state_.assignedSkills[i];
            Rml::Element* element = assignedSlotElements_[i];
            element->SetClass("occupied", skill >= 0);
            element->SetClass("selected", selectedSkillSlot_ == i);
            element->SetAttribute("data-skill-id", skill);
            assignedSlots_[i].SetEnabled(true);
            assignedSlots_[i].SetVisible(true);
            (void)assignedSlots_[i].Update();
        }

        const std::size_t count = std::min(
            availableSlots_.size(), state_.availableSkills.size());
        skillPicker_->SetProperty(
            "display", count > 0 ? "block" : "none");

        for (std::size_t i = 0; i < availableSlots_.size(); ++i)
        {
            const bool visible = i < count;
            availableSlots_[i].SetVisible(visible);
            availableSlots_[i].SetEnabled(visible);

            if (availableSlotElements_[i])
            {
                availableSlotElements_[i]->SetClass(
                    "occupied", visible);
                availableSlotElements_[i]->SetAttribute(
                    "data-skill-id",
                    visible ? state_.availableSkills[i] : -1);
            }

            (void)availableSlots_[i].Update();
        }
    }

    void SyncExtraItems(bool resetSelection)
    {
        if (resetSelection)
        {
            selectedExtraItem_.reset();
            itemScrollStart_ = 0;
        }

        const std::size_t maximum =
            state_.extraItems.size() > itemRowButtons_.size()
                ? state_.extraItems.size() - itemRowButtons_.size()
                : 0u;
        itemScrollStart_ = std::min(itemScrollStart_, maximum);

        for (std::size_t i = 0; i < itemRowElements_.size(); ++i)
        {
            Rml::Element* row = itemRowElements_[i];
            const std::size_t absolute = itemScrollStart_ + i;
            const bool visible = absolute < state_.extraItems.size();

            row->SetProperty("display", visible ? "block" : "none");
            row->SetInnerRML(
                visible ? state_.extraItems[absolute] : "");
            row->SetClass(
                "selected",
                visible &&
                selectedExtraItem_ &&
                *selectedExtraItem_ == absolute);

            itemRowButtons_[i].SetVisible(visible);
            itemRowButtons_[i].SetEnabled(visible);
            (void)itemRowButtons_[i].Update();
        }

        (void)itemScroll_.SetState(
            itemScrollStart_,
            maximum,
            itemRowButtons_.size(),
            itemRowButtons_.size(),
            maximum > 0,
            true);
    }

    [[nodiscard]] bool GetOption(Option option) const noexcept
    {
        switch (option)
        {
        case Option::FallbackBasic: return state_.fallbackBasic;
        case Option::LongCounter: return state_.longCounter;
        case Option::ReturnPosition: return state_.returnPosition;
        case Option::UsePotion: return state_.usePotion;
        case Option::SupportParty: return state_.supportParty;
        case Option::AutoHeal: return state_.autoHeal;
        case Option::DrainLife: return state_.drainLife;
        case Option::BuffDuration: return state_.buffDuration;
        case Option::UseCombo: return state_.useCombo;
        case Option::UseDarkRaven: return state_.useDarkRaven;
        case Option::RepairItem: return state_.repairItem;
        case Option::PickAll: return state_.pickAll;
        case Option::PickSelected: return state_.pickSelected;
        case Option::PickJewel: return state_.pickJewel;
        case Option::PickAncient: return state_.pickAncient;
        case Option::PickZen: return state_.pickZen;
        case Option::PickExcellent: return state_.pickExcellent;
        case Option::PickExtra: return state_.pickExtra;
        case Option::AcceptFriend: return state_.acceptFriend;
        case Option::AcceptGuild: return state_.acceptGuild;
        case Option::SelfDefense: return state_.selfDefense;
        }
        return false;
    }

    void SetOption(Option option, bool value) noexcept
    {
        switch (option)
        {
        case Option::FallbackBasic: state_.fallbackBasic = value; break;
        case Option::LongCounter: state_.longCounter = value; break;
        case Option::ReturnPosition: state_.returnPosition = value; break;
        case Option::UsePotion: state_.usePotion = value; break;
        case Option::SupportParty: state_.supportParty = value; break;
        case Option::AutoHeal: state_.autoHeal = value; break;
        case Option::DrainLife: state_.drainLife = value; break;
        case Option::BuffDuration: state_.buffDuration = value; break;
        case Option::UseCombo: state_.useCombo = value; break;
        case Option::UseDarkRaven: state_.useDarkRaven = value; break;
        case Option::RepairItem: state_.repairItem = value; break;
        case Option::PickAll: state_.pickAll = value; break;
        case Option::PickSelected: state_.pickSelected = value; break;
        case Option::PickJewel: state_.pickJewel = value; break;
        case Option::PickAncient: state_.pickAncient = value; break;
        case Option::PickZen: state_.pickZen = value; break;
        case Option::PickExcellent: state_.pickExcellent = value; break;
        case Option::PickExtra: state_.pickExtra = value; break;
        case Option::AcceptFriend: state_.acceptFriend = value; break;
        case Option::AcceptGuild: state_.acceptGuild = value; break;
        case Option::SelfDefense: state_.selfDefense = value; break;
        }
    }

    RmlDocumentHost host_;
    Rml::ElementDocument* document_ = nullptr;

    Rml::Element* root_ = nullptr;
    Rml::Element* drag_ = nullptr;
    Rml::Element* pageHunt_ = nullptr;
    Rml::Element* pageItem_ = nullptr;
    Rml::Element* pageOther_ = nullptr;
    Rml::Element* itemList_ = nullptr;
    Rml::Element* skillPicker_ = nullptr;

    Rml::ElementFormControlInput* returnSeconds_ = nullptr;
    Rml::ElementFormControlInput* manualYieldSeconds_ = nullptr;
    Rml::ElementFormControlInput* itemName_ = nullptr;

    std::array<Rml::Element*, 21> optionElements_{};
    std::array<Rml::Element*, 6> assignedSlotElements_{};
    std::array<Rml::Element*, 10> availableSlotElements_{};
    std::array<Rml::Element*, 5> itemRowElements_{};

    std::array<RmlMuButton, 3> tabButtons_{};
    std::array<RmlMuButton, 8> huntRangeButtons_{};
    std::array<RmlMuButton, 8> obtainRangeButtons_{};
    std::array<RmlMuButton, 21> optionButtons_{};
    std::array<RmlMuButton, 5> itemRowButtons_{};

    std::array<RmlMuSlot, 6> assignedSlots_{};
    std::array<RmlMuSlot, 10> availableSlots_{};

    RmlMuButton closeButton_;
    RmlMuButton resetButton_;
    RmlMuButton saveButton_;
    RmlMuButton itemAddButton_;
    RmlMuButton itemDeleteButton_;
    RmlMuButton potionConfigButton_;
    std::array<RmlMuButton, 2> skillConfigButtons_{};
    RmlMuButton partyConfigButton_;
    RmlMuScrollBar itemScroll_;
    RmlMuMovablePanel mover_;

    Design design_;
    State state_;
    std::size_t selectedSkillSlot_ = 0;
    std::size_t itemScrollStart_ = 0;
    std::optional<std::size_t> selectedExtraItem_;
    std::optional<Action> pendingAction_;
};

RmlMuHelperPanel::RmlMuHelperPanel()
    : m_impl(std::make_unique<Impl>())
{
}

RmlMuHelperPanel::~RmlMuHelperPanel() = default;
RmlMuHelperPanel::RmlMuHelperPanel(RmlMuHelperPanel&&) noexcept = default;
RmlMuHelperPanel& RmlMuHelperPanel::operator=(
    RmlMuHelperPanel&&) noexcept = default;

bool RmlMuHelperPanel::Load(bool show)
{
    return m_impl && m_impl->Load(show);
}

bool RmlMuHelperPanel::Show()
{
    return m_impl && m_impl->Show();
}

bool RmlMuHelperPanel::Hide()
{
    return m_impl && m_impl->Hide();
}

void RmlMuHelperPanel::Release()
{
    if (m_impl)
        m_impl->Release();
}

bool RmlMuHelperPanel::IsLoaded() const noexcept
{
    return m_impl && m_impl->IsLoaded();
}

bool RmlMuHelperPanel::ApplyState(const State& state)
{
    return m_impl && m_impl->ApplyState(state);
}

bool RmlMuHelperPanel::Update()
{
    return m_impl && m_impl->Update();
}

std::optional<RmlMuHelperPanel::Action>
RmlMuHelperPanel::ConsumeAction()
{
    return m_impl
        ? m_impl->ConsumeAction()
        : std::nullopt;
}

} // namespace UI::Modern::PC::MuHelper

#include "stdafx.h"
#include "UI/Modern/PC/MuHelper/RmlMuHelperPanel.h"

#include "UI/Modern/RmlDocumentHost.h"
#include "UI/Modern/RmlMuButton.h"
#include "UI/Modern/RmlMuMovablePanel.h"
#include "UI/Modern/RmlMuScrollBar.h"
#include "UI/Modern/RmlMuSlot.h"
#include "UI/Modern/RmlUiDesign.h"

#include <RmlUi/Core/Context.h>
#include <RmlUi/Core/Core.h>
#include <RmlUi/Core/Element.h>
#include <RmlUi/Core/ElementDocument.h>
#include <RmlUi/Core/FileInterface.h>
#include <RmlUi/Core/StringUtilities.h>
#include <RmlUi/Core/StreamMemory.h>
#include <RmlUi/Core/Elements/ElementFormControlInput.h>

#include <algorithm>
#include <array>
#include <charconv>
#include <memory>
#include <sstream>
#include <string_view>
#include <utility>
#include <vector>

namespace UI::Modern::PC::MuHelper
{

namespace
{
constexpr const char* kDocumentPath =
    "Data/UI/PC/MuHelper/mu_helper.rml";

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

template <typename T>
bool ParseScalar(const RmlUiDesign::Values& values,
                 const char* key, T& output)
{
    const auto it = values.find(key);
    if (it == values.end())
        return false;
    std::istringstream stream(it->second);
    T value{};
    if (!(stream >> value))
        return false;
    output = value;
    return true;
}

Rml::Element* ElementById(
    Rml::ElementDocument* document, const char* id)
{
    return document ? document->GetElementById(id) : nullptr;
}

Rml::ElementFormControlInput* InputById(
    Rml::ElementDocument* document, const char* id)
{
    return dynamic_cast<Rml::ElementFormControlInput*>(
        ElementById(document, id));
}

void SetChecked(Rml::ElementDocument* document,
                const char* id, bool checked)
{
    if (Rml::Element* element = ElementById(document, id))
    {
        if (checked)
            element->SetAttribute("checked", "");
        else
            element->RemoveAttribute("checked");
    }
}

bool IsChecked(Rml::ElementDocument* document, const char* id)
{
    if (Rml::Element* element = ElementById(document, id))
        return element->HasAttribute("checked");
    return false;
}

int ParseInt(Rml::ElementFormControlInput* input, int fallback)
{
    if (!input)
        return fallback;

    const std::string value = input->GetValue();
    int parsed = fallback;
    const auto [end, error] = std::from_chars(
        value.data(), value.data() + value.size(), parsed);
    if (error != std::errc{} || end != value.data() + value.size())
        return fallback;
    return parsed;
}

std::wstring Utf8ToWide(const std::string& input)
{
    std::wstring result;
    for (const char* p = input.data(), *end = p + input.size(); p < end;)
    {
        const Rml::Character codePoint =
            Rml::StringUtilities::ToCharacter(p, end);
        const std::size_t bytes =
            std::max<std::size_t>(1, Rml::StringUtilities::BytesUTF8(codePoint));
        p = std::min(end, p + bytes);

        if constexpr (sizeof(wchar_t) == 2)
        {
            if (codePoint <= 0xffff)
            {
                result.push_back(static_cast<wchar_t>(codePoint));
            }
            else
            {
                const char32_t value =
                    static_cast<char32_t>(codePoint) - 0x10000u;
                result.push_back(static_cast<wchar_t>(
                    0xd800u + (value >> 10u)));
                result.push_back(static_cast<wchar_t>(
                    0xdc00u + (value & 0x3ffu)));
            }
        }
        else
        {
            result.push_back(static_cast<wchar_t>(codePoint));
        }
    }
    return result;
}

std::string WideToUtf8(const std::wstring& input)
{
    std::string result;
    for (std::size_t i = 0; i < input.size(); ++i)
    {
        char32_t codePoint = static_cast<char32_t>(input[i]);
        if constexpr (sizeof(wchar_t) == 2)
        {
            const std::uint16_t first =
                static_cast<std::uint16_t>(input[i]);
            if (first >= 0xd800u && first <= 0xdbffu &&
                i + 1 < input.size())
            {
                const std::uint16_t second =
                    static_cast<std::uint16_t>(input[i + 1]);
                if (second >= 0xdc00u && second <= 0xdfffu)
                {
                    codePoint =
                        0x10000u +
                        ((static_cast<char32_t>(first - 0xd800u) << 10u) |
                         static_cast<char32_t>(second - 0xdc00u));
                    ++i;
                }
            }
        }
        result += Rml::StringUtilities::ToUTF8(
            static_cast<Rml::Character>(codePoint));
    }
    return result;
}

void SetEncodedText(Rml::Element* element, const std::string& text)
{
    if (element)
        element->SetInnerRML(Rml::StringUtilities::EncodeRml(text));
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
        float referenceWidth = 640.0f;
        float referenceHeight = 480.0f;
    };

    Impl() : host_(kDocumentPath) {}

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
        if (!Bind())
        {
            Release();
            return false;
        }

        visible_ = show;
        ApplyMetrics();
        ApplyConfigToDom();
        ApplyTab();
        RefreshExtraItems(true);
        return true;
    }

    [[nodiscard]] bool Show()
    {
        if (!host_.IsLoaded() && !Load(false))
            return false;
        visible_ = true;
        return host_.Show();
    }

    [[nodiscard]] bool Hide()
    {
        visible_ = false;
        return host_.Hide();
    }

    void Release()
    {
        mover_.Unbind();
        itemScroll_.Unbind();

        for (RmlMuButton* button : Buttons())
            button->Unbind();

        for (auto& button : huntRangeButtons_)
            if (button) button->Unbind();
        for (auto& button : obtainRangeButtons_)
            if (button) button->Unbind();
        for (auto& button : itemRowButtons_)
            if (button) button->Unbind();
        for (auto& slot : assignedSlots_)
            if (slot) slot->Unbind();
        for (auto& slot : availableSlots_)
            if (slot) slot->Unbind();

        huntRangeButtons_.clear();
        obtainRangeButtons_.clear();
        itemRowButtons_.clear();
        assignedSlots_.clear();
        availableSlots_.clear();

        root_ = nullptr;
        drag_ = nullptr;
        itemInput_ = nullptr;
        returnSecondsInput_ = nullptr;
        document_ = nullptr;
        host_.Release();

        pendingAction_.reset();
        pendingSkillSlot_.reset();
    }

    [[nodiscard]] bool IsLoaded() const noexcept
    {
        return host_.IsLoaded() && document_;
    }

    void SetConfig(const MUHelper::ConfigData& config)
    {
        config_ = config;
        extraItemsDirty_ = true;
        ApplyConfigToDom();
    }

    [[nodiscard]] MUHelper::ConfigData GetConfig() const
    {
        return config_;
    }

    void SetTab(Tab tab)
    {
        if (tab_ == tab)
            return;
        tab_ = tab;
        ApplyTab();
    }

    [[nodiscard]] Tab GetTab() const noexcept
    {
        return tab_;
    }

    void SetAssignedSkillFrame(std::size_t index, int frame)
    {
        if (index < assignedSlots_.size() && assignedSlots_[index])
            assignedSlots_[index]->SetIconFrame(frame);
    }

    void SetAvailableSkillFrame(std::size_t index, int frame)
    {
        if (index < availableSlots_.size() && availableSlots_[index])
            availableSlots_[index]->SetIconFrame(frame);
    }

    [[nodiscard]] bool Update()
    {
        if (!document_)
            return false;

        bool changed = false;
        for (RmlMuButton* button : Buttons())
            changed |= button->Update();

        changed |= PollRangeButtons();
        changed |= PollItemRows();
        changed |= PollSkillSlots();

        if (tabHuntButton_.ConsumeClicked())
        {
            tab_ = Tab::Hunt;
            ApplyTab();
            changed = true;
        }
        if (tabItemButton_.ConsumeClicked())
        {
            tab_ = Tab::Item;
            ApplyTab();
            changed = true;
        }
        if (tabOtherButton_.ConsumeClicked())
        {
            tab_ = Tab::Other;
            ApplyTab();
            changed = true;
        }

        if (resetButton_.ConsumeClicked())
        {
            pendingAction_ = Action::Reset;
            changed = true;
        }
        if (saveButton_.ConsumeClicked())
        {
            PullConfigFromDom();
            pendingAction_ = Action::Save;
            changed = true;
        }
        if (closeButton_.ConsumeClicked())
        {
            pendingAction_ = Action::Close;
            changed = true;
        }
        if (potionSettingsButton_.ConsumeClicked())
            pendingAction_ = Action::PotionSettings;
        if (skill1SettingsButton_.ConsumeClicked())
            pendingAction_ = Action::Skill1Settings;
        if (skill2SettingsButton_.ConsumeClicked())
            pendingAction_ = Action::Skill2Settings;
        if (partySettingsButton_.ConsumeClicked())
            pendingAction_ = Action::PartySettings;

        if (itemAddButton_.ConsumeClicked() && itemInput_)
        {
            const std::wstring item = Utf8ToWide(itemInput_->GetValue());
            if (!item.empty())
            {
                config_.aExtraItems.insert(item);
                itemInput_->SetValue("");
                extraItemsDirty_ = true;
                changed = true;
            }
        }

        if (itemDeleteButton_.ConsumeClicked() &&
            selectedExtraItem_ < config_.aExtraItems.size())
        {
            auto it = config_.aExtraItems.begin();
            std::advance(it, static_cast<std::ptrdiff_t>(selectedExtraItem_));
            config_.aExtraItems.erase(it);
            if (selectedExtraItem_ >= config_.aExtraItems.size() &&
                selectedExtraItem_ > 0)
                --selectedExtraItem_;
            extraItemsDirty_ = true;
            changed = true;
        }

        const std::size_t maximum =
            config_.aExtraItems.size() > 5
                ? config_.aExtraItems.size() - 5
                : 0;
        (void)itemScroll_.SetState(
            extraItemScroll_, maximum, 5, 5, maximum > 0, true);
        if (const auto request = itemScroll_.ConsumeRequestedPosition())
        {
            extraItemScroll_ = std::min(*request, maximum);
            extraItemsDirty_ = true;
            changed = true;
        }

        PullConfigFromDom();

        if (extraItemsDirty_)
        {
            RefreshExtraItems(false);
            changed = true;
        }

        changed |= mover_.ConsumePositionChanged();
        return changed ||
            pendingAction_.has_value() ||
            pendingSkillSlot_.has_value();
    }

    [[nodiscard]] std::optional<Action> ConsumeAction()
    {
        return std::exchange(pendingAction_, std::nullopt);
    }

    [[nodiscard]] std::optional<SkillSlotRequest> ConsumeSkillSlotRequest()
    {
        return std::exchange(pendingSkillSlot_, std::nullopt);
    }

private:
    void ReadDesign()
    {
        const auto values = LoadDesignValues();
        (void)ParseScalar(values, "MuHelper-Width", design_.width);
        (void)ParseScalar(values, "MuHelper-Height", design_.height);
        (void)ParseScalar(values, "MuHelper-SidePanelWidth", design_.sidePanelWidth);
        (void)ParseScalar(values, "MuHelper-ReferenceWidth", design_.referenceWidth);
        (void)ParseScalar(values, "MuHelper-ReferenceHeight", design_.referenceHeight);
    }

    [[nodiscard]] bool Bind()
    {
        root_ = ElementById(document_, "mu-helper");
        drag_ = ElementById(document_, "helper-drag");
        itemInput_ = InputById(document_, "item-name");
        returnSecondsInput_ = InputById(document_, "return-seconds");
        if (!root_ || !drag_ || !itemInput_ || !returnSecondsInput_)
            return false;

        itemInput_->SetAttribute("maxlength", 32);
        returnSecondsInput_->SetAttribute("maxlength", 4);

        auto bindButton = [&](RmlMuButton& button, const char* id) {
            Rml::Element* element = ElementById(document_, id);
            if (!element)
                return false;
            button.Bind(element);
            return true;
        };

        if (!bindButton(closeButton_, "helper-close") ||
            !bindButton(tabHuntButton_, "tab-hunt") ||
            !bindButton(tabItemButton_, "tab-item") ||
            !bindButton(tabOtherButton_, "tab-other") ||
            !bindButton(resetButton_, "helper-reset") ||
            !bindButton(saveButton_, "helper-save") ||
            !bindButton(itemAddButton_, "item-add") ||
            !bindButton(itemDeleteButton_, "item-delete") ||
            !bindButton(potionSettingsButton_, "potion-settings") ||
            !bindButton(skill1SettingsButton_, "skill-1-settings") ||
            !bindButton(skill2SettingsButton_, "skill-2-settings") ||
            !bindButton(partySettingsButton_, "party-settings"))
            return false;

        huntRangeButtons_.clear();
        obtainRangeButtons_.clear();
        for (int range = 1; range <= 8; ++range)
        {
            auto hunt = std::make_unique<RmlMuButton>();
            auto obtain = std::make_unique<RmlMuButton>();

            Rml::Element* huntElement = ElementById(
                document_, (Rml::String("hunt-range-") +
                            std::to_string(range)).c_str());
            Rml::Element* obtainElement = ElementById(
                document_, (Rml::String("obtain-range-") +
                            std::to_string(range)).c_str());
            if (!huntElement || !obtainElement)
                return false;

            hunt->Bind(huntElement);
            obtain->Bind(obtainElement);
            huntRangeButtons_.push_back(std::move(hunt));
            obtainRangeButtons_.push_back(std::move(obtain));
        }

        itemRowButtons_.clear();
        for (int row = 0; row < 5; ++row)
        {
            auto button = std::make_unique<RmlMuButton>();
            Rml::Element* element = ElementById(
                document_, (Rml::String("item-row-") +
                            std::to_string(row)).c_str());
            if (!element)
                return false;
            button->Bind(element);
            itemRowButtons_.push_back(std::move(button));
        }

        assignedSlots_.clear();
        for (int slot = 0; slot < 6; ++slot)
        {
            auto control = std::make_unique<RmlMuSlot>();
            Rml::Element* element = ElementById(
                document_, (Rml::String("assigned-skill-") +
                            std::to_string(slot)).c_str());
            if (!element)
                return false;
            control->Bind(element);
            assignedSlots_.push_back(std::move(control));
        }

        availableSlots_.clear();
        for (int slot = 0; slot < 10; ++slot)
        {
            auto control = std::make_unique<RmlMuSlot>();
            Rml::Element* element = ElementById(
                document_, (Rml::String("available-skill-") +
                            std::to_string(slot)).c_str());
            if (!element)
                return false;
            control->Bind(element);
            availableSlots_.push_back(std::move(control));
        }

        if (!itemScroll_.Bind(ElementById(document_, "item-scroll")))
            return false;

        mover_.Bind(root_, drag_);
        return true;
    }

    [[nodiscard]] std::array<RmlMuButton*, 12> Buttons()
    {
        return {
            &closeButton_,
            &tabHuntButton_, &tabItemButton_, &tabOtherButton_,
            &resetButton_, &saveButton_,
            &itemAddButton_, &itemDeleteButton_,
            &potionSettingsButton_, &skill1SettingsButton_,
            &skill2SettingsButton_, &partySettingsButton_
        };
    }

    void ApplyMetrics()
    {
        if (!document_ || !root_)
            return;

        const Rml::Vector2i dimensions =
            document_->GetContext()
                ? document_->GetContext()->GetDimensions()
                : Rml::Vector2i{};
        mover_.SetMetrics(
            static_cast<float>(dimensions.x),
            static_cast<float>(dimensions.y),
            design_.width,
            design_.height,
            0.0f,
            0.0f);
    }

    void ApplyTab()
    {
        if (!document_)
            return;

        const bool hunt = tab_ == Tab::Hunt;
        const bool item = tab_ == Tab::Item;
        const bool other = tab_ == Tab::Other;

        if (Rml::Element* page = ElementById(document_, "page-hunt"))
            page->SetProperty("display", hunt ? "block" : "none");
        if (Rml::Element* page = ElementById(document_, "page-item"))
            page->SetProperty("display", item ? "block" : "none");
        if (Rml::Element* page = ElementById(document_, "page-other"))
            page->SetProperty("display", other ? "block" : "none");

        if (Rml::Element* tab = ElementById(document_, "tab-hunt"))
            tab->SetClass("selected", hunt);
        if (Rml::Element* tab = ElementById(document_, "tab-item"))
            tab->SetClass("selected", item);
        if (Rml::Element* tab = ElementById(document_, "tab-other"))
            tab->SetClass("selected", other);
    }

    void ApplyConfigToDom()
    {
        if (!document_)
            return;

        SetChecked(document_, "fallback-basic", config_.bFallbackBasicAttack);
        SetChecked(document_, "long-counter", config_.bLongRangeCounterAttack);
        SetChecked(document_, "return-position", config_.bReturnToOriginalPosition);
        SetChecked(document_, "use-potion", config_.bUseHealPotion);
        SetChecked(document_, "combo", config_.bUseCombo);
        SetChecked(document_, "use-dark-raven", config_.bUseDarkRaven);
        SetChecked(document_, "support-party", config_.bSupportParty);
        SetChecked(document_, "auto-heal", config_.bAutoHeal);
        SetChecked(document_, "drain-life", config_.bUseDrainLife);
        SetChecked(document_, "buff-duration", config_.bBuffDuration);

        SetChecked(document_, "repair-item", config_.bRepairItem);
        SetChecked(document_, "pick-all", config_.bPickAllItems);
        SetChecked(document_, "pick-selected", config_.bPickSelectItems);
        SetChecked(document_, "pick-jewel", config_.bPickJewel);
        SetChecked(document_, "pick-ancient", config_.bPickAncient);
        SetChecked(document_, "pick-zen", config_.bPickZen);
        SetChecked(document_, "pick-excellent", config_.bPickExcellent);
        SetChecked(document_, "pick-extra", config_.bPickExtraItems);

        SetChecked(document_, "accept-friend", config_.bAutoAcceptFriend);
        SetChecked(document_, "accept-guild", config_.bAutoAcceptGuild);
        SetChecked(document_, "self-defense", config_.bUseSelfDefense);

        for (int mode = 0; mode <= 2; ++mode)
            SetChecked(
                document_,
                (std::string("dark-mode-") + std::to_string(mode)).c_str(),
                config_.iDarkRavenMode == mode);

        if (returnSecondsInput_)
            returnSecondsInput_->SetValue(
                std::to_string(config_.iMaxSecondsAway));

        SyncRangeClasses();
        extraItemsDirty_ = true;
    }

    void PullConfigFromDom()
    {
        if (!document_)
            return;

        config_.bFallbackBasicAttack = IsChecked(document_, "fallback-basic");
        config_.bLongRangeCounterAttack = IsChecked(document_, "long-counter");
        config_.bReturnToOriginalPosition = IsChecked(document_, "return-position");
        config_.bUseHealPotion = IsChecked(document_, "use-potion");
        config_.bUseCombo = IsChecked(document_, "combo");
        config_.bUseDarkRaven = IsChecked(document_, "use-dark-raven");
        config_.bSupportParty = IsChecked(document_, "support-party");
        config_.bAutoHeal = IsChecked(document_, "auto-heal");
        config_.bUseDrainLife = IsChecked(document_, "drain-life");
        config_.bBuffDuration = IsChecked(document_, "buff-duration");

        config_.bRepairItem = IsChecked(document_, "repair-item");
        config_.bPickAllItems = IsChecked(document_, "pick-all");
        config_.bPickSelectItems = IsChecked(document_, "pick-selected");
        config_.bPickJewel = IsChecked(document_, "pick-jewel");
        config_.bPickAncient = IsChecked(document_, "pick-ancient");
        config_.bPickZen = IsChecked(document_, "pick-zen");
        config_.bPickExcellent = IsChecked(document_, "pick-excellent");
        config_.bPickExtraItems = IsChecked(document_, "pick-extra");

        config_.bAutoAcceptFriend = IsChecked(document_, "accept-friend");
        config_.bAutoAcceptGuild = IsChecked(document_, "accept-guild");
        config_.bUseSelfDefense = IsChecked(document_, "self-defense");

        for (int mode = 0; mode <= 2; ++mode)
        {
            if (IsChecked(
                    document_,
                    (std::string("dark-mode-") + std::to_string(mode)).c_str()))
            {
                config_.iDarkRavenMode = mode;
                break;
            }
        }

        config_.iMaxSecondsAway =
            std::max(0, ParseInt(returnSecondsInput_, config_.iMaxSecondsAway));
    }

    [[nodiscard]] bool PollRangeButtons()
    {
        bool changed = false;
        for (std::size_t i = 0; i < huntRangeButtons_.size(); ++i)
        {
            RmlMuButton* button = huntRangeButtons_[i].get();
            if (!button)
                continue;
            changed |= button->Update();
            if (button->ConsumeClicked())
            {
                config_.iHuntingRange = static_cast<int>(i + 1);
                changed = true;
            }
        }
        for (std::size_t i = 0; i < obtainRangeButtons_.size(); ++i)
        {
            RmlMuButton* button = obtainRangeButtons_[i].get();
            if (!button)
                continue;
            changed |= button->Update();
            if (button->ConsumeClicked())
            {
                config_.iObtainingRange = static_cast<int>(i + 1);
                changed = true;
            }
        }
        if (changed)
            SyncRangeClasses();
        return changed;
    }

    void SyncRangeClasses()
    {
        if (!document_)
            return;

        for (int range = 1; range <= 8; ++range)
        {
            if (Rml::Element* element = ElementById(
                    document_,
                    (std::string("hunt-range-") +
                     std::to_string(range)).c_str()))
            {
                element->SetClass(
                    "selected", config_.iHuntingRange == range);
            }
            if (Rml::Element* element = ElementById(
                    document_,
                    (std::string("obtain-range-") +
                     std::to_string(range)).c_str()))
            {
                element->SetClass(
                    "selected", config_.iObtainingRange == range);
            }
        }
    }

    [[nodiscard]] bool PollItemRows()
    {
        bool changed = false;
        for (std::size_t row = 0; row < itemRowButtons_.size(); ++row)
        {
            RmlMuButton* button = itemRowButtons_[row].get();
            if (!button)
                continue;
            changed |= button->Update();
            if (button->ConsumeClicked())
            {
                const std::size_t index = extraItemScroll_ + row;
                if (index < config_.aExtraItems.size())
                {
                    selectedExtraItem_ = index;
                    extraItemsDirty_ = true;
                    changed = true;
                }
            }
        }
        return changed;
    }

    [[nodiscard]] bool PollSkillSlots()
    {
        bool changed = false;
        for (std::size_t i = 0; i < assignedSlots_.size(); ++i)
        {
            RmlMuSlot* slot = assignedSlots_[i].get();
            if (!slot)
                continue;
            changed |= slot->Update();
            if (slot->ConsumeClicked() || slot->ConsumeSecondaryClicked())
            {
                pendingSkillSlot_ = SkillSlotRequest{false, i};
                changed = true;
            }
        }
        for (std::size_t i = 0; i < availableSlots_.size(); ++i)
        {
            RmlMuSlot* slot = availableSlots_[i].get();
            if (!slot)
                continue;
            changed |= slot->Update();
            if (slot->ConsumeClicked() || slot->ConsumeSecondaryClicked())
            {
                pendingSkillSlot_ = SkillSlotRequest{true, i};
                changed = true;
            }
        }
        return changed;
    }

    void RefreshExtraItems(bool force)
    {
        if (!document_ || (!extraItemsDirty_ && !force))
            return;
        extraItemsDirty_ = false;

        const std::size_t maximum =
            config_.aExtraItems.size() > 5
                ? config_.aExtraItems.size() - 5
                : 0;
        extraItemScroll_ = std::min(extraItemScroll_, maximum);

        std::vector<std::wstring> items(
            config_.aExtraItems.begin(), config_.aExtraItems.end());

        for (std::size_t row = 0; row < 5; ++row)
        {
            Rml::Element* element = ElementById(
                document_,
                (std::string("item-row-") +
                 std::to_string(row)).c_str());
            if (!element)
                continue;

            const std::size_t index = extraItemScroll_ + row;
            if (index < items.size())
            {
                element->SetProperty("display", "block");
                SetEncodedText(element, WideToUtf8(items[index]));
                element->SetClass("selected", selectedExtraItem_ == index);
            }
            else
            {
                element->SetProperty("display", "none");
                element->SetClass("selected", false);
            }
        }
    }

    RmlDocumentHost host_;
    Rml::ElementDocument* document_ = nullptr;
    Rml::Element* root_ = nullptr;
    Rml::Element* drag_ = nullptr;
    Rml::ElementFormControlInput* itemInput_ = nullptr;
    Rml::ElementFormControlInput* returnSecondsInput_ = nullptr;

    RmlMuMovablePanel mover_;
    RmlMuScrollBar itemScroll_;

    RmlMuButton closeButton_;
    RmlMuButton tabHuntButton_;
    RmlMuButton tabItemButton_;
    RmlMuButton tabOtherButton_;
    RmlMuButton resetButton_;
    RmlMuButton saveButton_;
    RmlMuButton itemAddButton_;
    RmlMuButton itemDeleteButton_;
    RmlMuButton potionSettingsButton_;
    RmlMuButton skill1SettingsButton_;
    RmlMuButton skill2SettingsButton_;
    RmlMuButton partySettingsButton_;

    std::vector<std::unique_ptr<RmlMuButton>> huntRangeButtons_;
    std::vector<std::unique_ptr<RmlMuButton>> obtainRangeButtons_;
    std::vector<std::unique_ptr<RmlMuButton>> itemRowButtons_;
    std::vector<std::unique_ptr<RmlMuSlot>> assignedSlots_;
    std::vector<std::unique_ptr<RmlMuSlot>> availableSlots_;

    Design design_;
    MUHelper::ConfigData config_;
    Tab tab_ = Tab::Hunt;
    bool visible_ = false;

    std::size_t extraItemScroll_ = 0;
    std::size_t selectedExtraItem_ = 0;
    bool extraItemsDirty_ = true;

    std::optional<Action> pendingAction_;
    std::optional<SkillSlotRequest> pendingSkillSlot_;
};

RmlMuHelperPanel::RmlMuHelperPanel()
    : m_impl(std::make_unique<Impl>())
{
}

RmlMuHelperPanel::~RmlMuHelperPanel() = default;
RmlMuHelperPanel::RmlMuHelperPanel(RmlMuHelperPanel&&) noexcept = default;
RmlMuHelperPanel& RmlMuHelperPanel::operator=(RmlMuHelperPanel&&) noexcept = default;

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

void RmlMuHelperPanel::SetConfig(const MUHelper::ConfigData& config)
{
    if (m_impl)
        m_impl->SetConfig(config);
}

MUHelper::ConfigData RmlMuHelperPanel::GetConfig() const
{
    return m_impl ? m_impl->GetConfig() : MUHelper::ConfigData{};
}

void RmlMuHelperPanel::SetTab(Tab tab)
{
    if (m_impl)
        m_impl->SetTab(tab);
}

RmlMuHelperPanel::Tab RmlMuHelperPanel::GetTab() const noexcept
{
    return m_impl ? m_impl->GetTab() : Tab::Hunt;
}

void RmlMuHelperPanel::SetAssignedSkillFrame(
    std::size_t index, int frame)
{
    if (m_impl)
        m_impl->SetAssignedSkillFrame(index, frame);
}

void RmlMuHelperPanel::SetAvailableSkillFrame(
    std::size_t index, int frame)
{
    if (m_impl)
        m_impl->SetAvailableSkillFrame(index, frame);
}

bool RmlMuHelperPanel::Update()
{
    return m_impl && m_impl->Update();
}

std::optional<RmlMuHelperPanel::Action>
RmlMuHelperPanel::ConsumeAction()
{
    return m_impl ? m_impl->ConsumeAction() : std::nullopt;
}

std::optional<RmlMuHelperPanel::SkillSlotRequest>
RmlMuHelperPanel::ConsumeSkillSlotRequest()
{
    return m_impl
        ? m_impl->ConsumeSkillSlotRequest()
        : std::nullopt;
}

} // namespace UI::Modern::PC::MuHelper

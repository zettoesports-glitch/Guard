#pragma once

#include <array>
#include <cstddef>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace UI::Modern::PC::MuHelper
{

class RmlMuHelperPanel
{
public:
    enum class Tab
    {
        Hunt = 0,
        Item = 1,
        Other = 2,
    };

    enum class Option
    {
        FallbackBasic,
        LongCounter,
        ReturnPosition,
        UsePotion,
        SupportParty,
        AutoHeal,
        DrainLife,
        BuffDuration,
        UseCombo,
        UseDarkRaven,
        RepairItem,
        PickAll,
        PickSelected,
        PickJewel,
        PickAncient,
        PickZen,
        PickExcellent,
        PickExtra,
        AcceptFriend,
        AcceptGuild,
        SelfDefense,
    };

    struct State
    {
        bool visible = false;
        int viewportWidth = 0;
        int viewportHeight = 0;

        Tab tab = Tab::Hunt;
        int huntRange = 1;
        int obtainingRange = 1;
        int maxSecondsAway = 0;
        int manualYieldSeconds = 0;

        bool fallbackBasic = true;
        bool longCounter = false;
        bool returnPosition = false;
        bool usePotion = false;
        bool supportParty = false;
        bool autoHeal = false;
        bool drainLife = false;
        bool buffDuration = false;
        bool useCombo = false;
        bool useDarkRaven = false;

        bool repairItem = false;
        bool pickAll = false;
        bool pickSelected = false;
        bool pickJewel = false;
        bool pickAncient = false;
        bool pickZen = false;
        bool pickExcellent = false;
        bool pickExtra = false;

        bool acceptFriend = false;
        bool acceptGuild = false;
        bool selfDefense = false;

        std::array<int, 6> assignedSkills{
            -1, -1, -1, -1, -1, -1};
        std::vector<int> availableSkills;
        std::vector<std::string> extraItems;
    };

    enum class ActionType
    {
        Close,
        Reset,
        Save,
        SelectTab,
        SetHuntRange,
        SetObtainingRange,
        ToggleOption,
        ConfigurePotion,
        ConfigureSkill,
        ConfigureParty,
        SelectAssignedSkill,
        RemoveAssignedSkill,
        ChooseAvailableSkill,
        AddExtraItem,
        DeleteExtraItem,
    };

    struct Action
    {
        ActionType type = ActionType::Close;
        Option option = Option::FallbackBasic;
        std::size_t index = 0;
        int intValue = 0;
        bool boolValue = false;
        std::string text;
    };

    RmlMuHelperPanel();
    ~RmlMuHelperPanel();

    RmlMuHelperPanel(const RmlMuHelperPanel&) = delete;
    RmlMuHelperPanel& operator=(const RmlMuHelperPanel&) = delete;
    RmlMuHelperPanel(RmlMuHelperPanel&&) noexcept;
    RmlMuHelperPanel& operator=(RmlMuHelperPanel&&) noexcept;

    [[nodiscard]] bool Load(bool show = false);
    [[nodiscard]] bool Show();
    [[nodiscard]] bool Hide();
    void Release();

    [[nodiscard]] bool IsLoaded() const noexcept;
    [[nodiscard]] bool ApplyState(const State& state);
    [[nodiscard]] bool Update();
    [[nodiscard]] std::optional<Action> ConsumeAction();

private:
    class Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace UI::Modern::PC::MuHelper

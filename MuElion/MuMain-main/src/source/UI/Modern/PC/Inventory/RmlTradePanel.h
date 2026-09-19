#pragma once

#include <array>
#include <cstddef>
#include <memory>
#include <optional>
#include <string>

namespace UI::Modern::PC::Inventory
{

class RmlTradePanel
{
public:
    static constexpr std::size_t kColumns = 8;
    static constexpr std::size_t kRows = 4;
    static constexpr std::size_t kSlotsPerSide = kColumns * kRows;

    struct SlotState
    {
        bool visible = true;
        bool enabled = true;
        bool occupied = false;
        bool selected = false;
        bool warning = false;
        int iconFrame = 0;
    };

    struct State
    {
        bool visible = false;
        int viewportWidth = 0;
        int viewportHeight = 0;

        std::string title = "Trade";
        std::string otherName;
        std::string otherGuild;
        std::string otherLevel;
        std::string otherZen;
        std::string myName;
        std::string myZen;
        std::string cautionText;

        bool myConfirmed = false;
        bool otherConfirmed = false;
        bool tradeAlert = false;
        bool confirmEnabled = true;

        std::array<SlotState, kSlotsPerSide> otherSlots{};
        std::array<SlotState, kSlotsPerSide> mySlots{};
    };

    enum class ActionType
    {
        Close,
        InputZen,
        ToggleConfirm,
        MySlotPrimary,
        MySlotSecondary,
        OtherSlotInspect,
    };

    struct Action
    {
        ActionType type = ActionType::Close;
        std::size_t index = 0;
    };

    RmlTradePanel();
    ~RmlTradePanel();

    RmlTradePanel(const RmlTradePanel&) = delete;
    RmlTradePanel& operator=(const RmlTradePanel&) = delete;
    RmlTradePanel(RmlTradePanel&&) noexcept;
    RmlTradePanel& operator=(RmlTradePanel&&) noexcept;

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

} // namespace UI::Modern::PC::Inventory

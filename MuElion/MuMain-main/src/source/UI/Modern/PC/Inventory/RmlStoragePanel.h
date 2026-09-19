#pragma once

#include <array>
#include <cstddef>
#include <memory>
#include <optional>
#include <string>

namespace UI::Modern::PC::Inventory
{

class RmlStoragePanel
{
public:
    static constexpr std::size_t kColumns = 8;
    static constexpr std::size_t kRows = 15;
    static constexpr std::size_t kSlotCount = kColumns * kRows;

    struct SlotState
    {
        bool visible = true;
        bool enabled = true;
        bool occupied = false;
        bool selected = false;
        int iconFrame = 0;
    };

    struct State
    {
        bool visible = false;
        int viewportWidth = 0;
        int viewportHeight = 0;

        std::string title = "Storage";
        std::string storedZen;
        std::string charge;
        std::string remainingTime;

        bool locked = false;
        bool chargeVisible = true;
        bool periodVisible = false;
        bool extendEnabled = false;

        std::array<SlotState, kSlotCount> slots{};
    };

    enum class ActionType
    {
        Close,
        Exit,
        DepositZen,
        WithdrawZen,
        ToggleLock,
        Extend,
        ZenInfo,
        SlotPrimary,
        SlotSecondary,
    };

    struct Action
    {
        ActionType type = ActionType::Close;
        std::size_t index = 0;
    };

    RmlStoragePanel();
    ~RmlStoragePanel();

    RmlStoragePanel(const RmlStoragePanel&) = delete;
    RmlStoragePanel& operator=(const RmlStoragePanel&) = delete;
    RmlStoragePanel(RmlStoragePanel&&) noexcept;
    RmlStoragePanel& operator=(RmlStoragePanel&&) noexcept;

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

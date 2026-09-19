#pragma once

#include <array>
#include <cstddef>
#include <memory>
#include <optional>
#include <string>

namespace UI::Modern::PC::Inventory
{

class RmlInventoryExtensionPanel
{
public:
    static constexpr std::size_t kBagCount = 4;
    static constexpr std::size_t kSlotsPerBag = 32;
    static constexpr std::size_t kSlotCount =
        kBagCount * kSlotsPerBag;

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
        std::size_t openedBags = 0;
        std::string title = "Expanded Inventory";
        std::array<SlotState, kSlotCount> slots{};
    };

    enum class ActionType
    {
        Close,
        SlotPrimary,
        SlotSecondary,
    };

    struct Action
    {
        ActionType type = ActionType::Close;
        std::size_t index = 0;
    };

    RmlInventoryExtensionPanel();
    ~RmlInventoryExtensionPanel();

    RmlInventoryExtensionPanel(
        const RmlInventoryExtensionPanel&) = delete;
    RmlInventoryExtensionPanel& operator=(
        const RmlInventoryExtensionPanel&) = delete;
    RmlInventoryExtensionPanel(
        RmlInventoryExtensionPanel&&) noexcept;
    RmlInventoryExtensionPanel& operator=(
        RmlInventoryExtensionPanel&&) noexcept;

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

#pragma once

#include <array>
#include <cstddef>
#include <memory>
#include <optional>
#include <string>

namespace UI::Modern::PC::Inventory
{

class RmlInventoryPanel
{
public:
    static constexpr std::size_t kInventorySlotCount = 64;
    static constexpr std::size_t kEquipmentSlotCount = 12;

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

        std::string title = "Inventory";
        std::string zen;

        bool repairEnabled = true;
        bool repairActive = false;
        bool privateStoreEnabled = true;
        bool privateStoreOpen = false;
        bool extensionEnabled = true;
        bool socketOptionEnabled = true;
        bool setOptionEnabled = true;

        std::array<SlotState, kInventorySlotCount> inventory{};
        std::array<SlotState, kEquipmentSlotCount> equipment{};
    };

    enum class ActionType
    {
        Close,
        Repair,
        PrivateStore,
        Extension,
        SocketOption,
        SetOption,
        InventorySlotPrimary,
        InventorySlotSecondary,
        EquipmentSlotPrimary,
        EquipmentSlotSecondary,
    };

    struct Action
    {
        ActionType type = ActionType::Close;
        std::size_t index = 0;
    };

    RmlInventoryPanel();
    ~RmlInventoryPanel();

    RmlInventoryPanel(const RmlInventoryPanel&) = delete;
    RmlInventoryPanel& operator=(const RmlInventoryPanel&) = delete;
    RmlInventoryPanel(RmlInventoryPanel&&) noexcept;
    RmlInventoryPanel& operator=(RmlInventoryPanel&&) noexcept;

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

#pragma once

#include <array>
#include <cstddef>
#include <memory>
#include <optional>
#include <string>

namespace UI::Modern::PC::Inventory
{

class RmlStorageExtensionPanel
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
        std::string title = "Expanded Storage";
        std::array<SlotState, kSlotCount> slots{};
    };

    enum class ActionType
    {
        Close,
        Exit,
        SlotPrimary,
        SlotSecondary,
    };

    struct Action
    {
        ActionType type = ActionType::Close;
        std::size_t index = 0;
    };

    RmlStorageExtensionPanel();
    ~RmlStorageExtensionPanel();

    RmlStorageExtensionPanel(const RmlStorageExtensionPanel&) = delete;
    RmlStorageExtensionPanel& operator=(const RmlStorageExtensionPanel&) = delete;
    RmlStorageExtensionPanel(RmlStorageExtensionPanel&&) noexcept;
    RmlStorageExtensionPanel& operator=(RmlStorageExtensionPanel&&) noexcept;

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

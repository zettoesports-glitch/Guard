#pragma once

#include <array>
#include <cstddef>
#include <memory>
#include <optional>
#include <string>

namespace UI::Modern::PC::Inventory
{

class RmlPrivateStorePanel
{
public:
    static constexpr std::size_t kSlotCount = 32;

    enum class Mode
    {
        Seller,
        Buyer,
    };

    enum class DropState
    {
        None,
        Allowed,
        Banned,
    };

    struct SlotState
    {
        bool visible = true;
        bool enabled = true;
        bool occupied = false;
        bool selected = false;
        int iconFrame = 0;
        DropState dropState = DropState::None;
    };

    struct State
    {
        bool visible = false;
        int viewportWidth = 0;
        int viewportHeight = 0;

        Mode mode = Mode::Seller;
        bool shopOpen = false;

        std::string title = "Private Store";
        std::string storeName;
        std::string buyerName;

        std::string openLabel = "Open";
        std::string closeShopLabel = "Close Shop";

        bool openEnabled = true;
        bool closeShopEnabled = true;

        std::array<SlotState, kSlotCount> slots{};
    };

    enum class ActionType
    {
        CloseWindow,
        OpenShop,
        CloseShop,
        SlotPrimary,
        SlotSecondary,
    };

    struct Action
    {
        ActionType type = ActionType::CloseWindow;
        std::size_t index = 0;
        std::string text;
    };

    RmlPrivateStorePanel();
    ~RmlPrivateStorePanel();

    RmlPrivateStorePanel(const RmlPrivateStorePanel&) = delete;
    RmlPrivateStorePanel& operator=(const RmlPrivateStorePanel&) = delete;
    RmlPrivateStorePanel(RmlPrivateStorePanel&&) noexcept;
    RmlPrivateStorePanel& operator=(RmlPrivateStorePanel&&) noexcept;

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

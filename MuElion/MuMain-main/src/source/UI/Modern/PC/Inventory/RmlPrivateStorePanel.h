#pragma once

#include <cstddef>
#include <memory>
#include <optional>
#include <string>

namespace UI::Modern::PC::Inventory
{

class RmlPrivateStorePanel
{
public:
    enum class Mode
    {
        Seller,
        Buyer,
    };

    enum class Action
    {
        OpenStore,
        CloseShop,
        ClosePanel,
    };

    enum class DropState
    {
        Neutral,
        Allowed,
        Banned,
    };

    struct SlotRequest
    {
        std::size_t index = 0;
        bool secondary = false;
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

    void SetMode(Mode mode);
    [[nodiscard]] Mode GetMode() const noexcept;

    void SetStoreName(std::string name);
    [[nodiscard]] std::string GetStoreName() const;

    void SetBuyerName(std::string name);
    void SetTitle(std::string title);

    void SetOpenEnabled(bool enabled);
    void SetCloseShopEnabled(bool enabled);

    void SetSlotFrame(std::size_t index, int frame);
    void SetSlotDropState(std::size_t index, DropState state);

    [[nodiscard]] bool Update();
    [[nodiscard]] std::optional<Action> ConsumeAction();
    [[nodiscard]] std::optional<SlotRequest> ConsumeSlotRequest();

private:
    class Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace UI::Modern::PC::Inventory

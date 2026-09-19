#pragma once

#include <cstddef>
#include <memory>
#include <optional>
#include <string>

namespace UI::Modern::PC::Inventory
{

class RmlInventoryPanel
{
public:
    enum class Action
    {
        Close,
        SocketOption,
        SetOption,
        Repair,
        PrivateStore,
        ExtensionBag,
        CloseExtension,
    };

    enum class SlotArea
    {
        Inventory,
        Equipment,
        Extension,
    };

    struct SlotRequest
    {
        SlotArea area = SlotArea::Inventory;
        std::size_t index = 0;
        bool secondary = false;
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

    [[nodiscard]] bool ShowExtension(bool show);
    [[nodiscard]] bool IsExtensionLoaded() const noexcept;

    void SetTitle(std::string title);
    void SetZenText(std::string zenText);

    // 0 hides the extension. 1..4 exposes 32 slots per unlocked bag.
    void SetExpandedBagCount(std::size_t bagCount);
    [[nodiscard]] std::size_t GetExpandedBagCount() const noexcept;

    void SetInventorySlotFrame(std::size_t index, int frame);
    void SetEquipmentSlotFrame(std::size_t index, int frame);
    void SetExtensionSlotFrame(std::size_t index, int frame);

    [[nodiscard]] bool Update();
    [[nodiscard]] std::optional<Action> ConsumeAction();
    [[nodiscard]] std::optional<SlotRequest> ConsumeSlotRequest();

private:
    class Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace UI::Modern::PC::Inventory

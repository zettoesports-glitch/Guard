#include "stdafx.h"
#include "UI/Modern/PC/Inventory/RmlPrivateStoreLegacyBridge.h"

#include "UI/Modern/PC/Inventory/RmlPrivateStorePanel.h"

#include "Render/Textures/ZzzOpenglUtil.h"
#include "UI/NewUI/Inventory/NewUIInventoryCtrl.h"
#include "UI/NewUI/Inventory/NewUIMyShopInventory.h"
#include "UI/NewUI/Inventory/NewUIPurchaseShopInventory.h"
#include "UI/NewUI/NewUISystem.h"

#include <RmlUi/Core/StringUtilities.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <string>

namespace UI::Modern::PC::Inventory
{

namespace
{

std::string WideToUtf8(const std::wstring& input)
{
    std::string result;
    result.reserve(input.size());

    for (std::size_t index = 0; index < input.size(); ++index)
    {
        char32_t codePoint =
            static_cast<char32_t>(input[index]);

        if constexpr (sizeof(wchar_t) == 2)
        {
            const std::uint16_t first =
                static_cast<std::uint16_t>(input[index]);
            if (first >= 0xd800u && first <= 0xdbffu &&
                index + 1 < input.size())
            {
                const std::uint16_t second =
                    static_cast<std::uint16_t>(input[index + 1]);
                if (second >= 0xdc00u && second <= 0xdfffu)
                {
                    codePoint =
                        0x10000u +
                        ((static_cast<char32_t>(first - 0xd800u) << 10u) |
                         static_cast<char32_t>(second - 0xdc00u));
                    ++index;
                }
            }
        }

        result += Rml::StringUtilities::ToUTF8(
            static_cast<Rml::Character>(codePoint));
    }

    return result;
}

void FillSlots(
    RmlPrivateStorePanel::State& state,
    SEASON3B::CNewUIInventoryCtrl* control,
    int pointed)
{
    for (std::size_t index = 0;
         index < state.slots.size();
         ++index)
    {
        const int column = static_cast<int>(index % 8u);
        const int row = static_cast<int>(index / 8u);

        auto& slot = state.slots[index];
        slot.visible = true;
        slot.enabled = true;
        slot.iconFrame = 0;
        slot.dropState =
            RmlPrivateStorePanel::DropState::None;

        if (!control)
        {
            slot.occupied = false;
            slot.selected = false;
            continue;
        }

        slot.occupied =
            control->FindItem(column, row) != nullptr;
        slot.selected =
            pointed == control->GetIndex(column, row);
    }
}

RmlPrivateStorePanel::State BuildState()
{
    RmlPrivateStorePanel::State state;
    state.viewportWidth = static_cast<int>(WindowWidth);
    state.viewportHeight = static_cast<int>(WindowHeight);

    if (!g_pNewUISystem)
        return state;

    const bool sellerVisible =
        g_pNewUISystem->IsVisible(
            SEASON3B::INTERFACE_MYSHOP_INVENTORY);
    const bool buyerVisible =
        g_pNewUISystem->IsVisible(
            SEASON3B::INTERFACE_PURCHASESHOP_INVENTORY);

    state.visible = sellerVisible || buyerVisible;

    if (buyerVisible && g_pPurchaseShopInventory)
    {
        state.mode = RmlPrivateStorePanel::Mode::Buyer;
        state.shopOpen = true;
        state.openEnabled = false;
        state.closeShopEnabled = false;
        state.buyerName =
            WideToUtf8(g_pPurchaseShopInventory->GetTitleText());

        FillSlots(
            state,
            g_pPurchaseShopInventory->GetInventoryCtrl(),
            g_pPurchaseShopInventory->GetPointedItemIndex());
        return state;
    }

    state.mode = RmlPrivateStorePanel::Mode::Seller;

    if (!g_pMyShopInventory)
        return state;

    state.shopOpen =
        g_pMyShopInventory->IsEnablePersonalShop();
    state.openEnabled = !state.shopOpen;
    state.closeShopEnabled = state.shopOpen;

    std::array<wchar_t, 256> title{};
    g_pMyShopInventory->GetTitle(title.data());
    state.storeName =
        WideToUtf8(std::wstring(title.data()));

    FillSlots(
        state,
        g_pMyShopInventory->GetInventoryCtrl(),
        g_pMyShopInventory->GetPointedItemIndex());

    return state;
}

} // namespace

bool RmlPrivateStoreLegacyBridge::Synchronize(
    RmlPrivateStorePanel& panel) const
{
    if (!panel.IsLoaded())
        return false;

    return panel.ApplyState(BuildState());
}

} // namespace UI::Modern::PC::Inventory

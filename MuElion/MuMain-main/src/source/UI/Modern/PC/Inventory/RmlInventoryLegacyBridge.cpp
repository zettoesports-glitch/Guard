#include "stdafx.h"
#include "UI/Modern/PC/Inventory/RmlInventoryLegacyBridge.h"

#include "UI/Modern/PC/Inventory/RmlInventoryPanel.h"

#include "Engine/Object/ZzzInfomation.h"
#include "Render/Textures/ZzzOpenglUtil.h"
#include "UI/NewUI/Inventory/NewUIInventoryCtrl.h"
#include "UI/NewUI/Inventory/NewUIMyInventory.h"
#include "UI/NewUI/NewUISystem.h"

#include <algorithm>
#include <cstddef>

namespace UI::Modern::PC::Inventory
{

namespace
{

RmlInventoryPanel::State BuildState()
{
    RmlInventoryPanel::State state;
    state.viewportWidth = static_cast<int>(WindowWidth);
    state.viewportHeight = static_cast<int>(WindowHeight);

    if (!g_pNewUISystem || !g_pMyInventory)
        return state;

    state.visible =
        g_pNewUISystem->IsVisible(SEASON3B::INTERFACE_INVENTORY);

    state.repairEnabled =
        g_pMyInventory->IsRepairEnableLevel();
    state.repairActive =
        g_pMyInventory->GetRepairMode() ==
        SEASON3B::REPAIR_MODE_ON;

    state.privateStoreEnabled =
        CharacterAttribute &&
        CharacterAttribute->Level >= 6;
    state.privateStoreOpen =
        g_pNewUISystem->IsVisible(
            SEASON3B::INTERFACE_MYSHOP_INVENTORY);

    // The legacy main inventory exposes an expanded-inventory button whenever
    // the extension controller is part of NewUISystem. Keep the presentation
    // enabled; actual open/close policy stays in legacy NewUISystem.
    state.extensionEnabled = true;

    // Legacy renders both informational set/socket option affordances even
    // when their content is inactive (grey text). Keep them interactable at
    // the presentation boundary; the action bridge will defer validation to
    // the legacy managers.
    state.socketOptionEnabled = true;
    state.setOptionEnabled = true;

    SEASON3B::CNewUIInventoryCtrl* control =
        g_pMyInventory->GetInventoryCtrl();
    if (control)
    {
        const int pointed = control->GetPointedSquareIndex();

        for (std::size_t index = 0;
             index < state.inventory.size();
             ++index)
        {
            const int column = static_cast<int>(index % 8u);
            const int row = static_cast<int>(index / 8u);

            ITEM* item = control->FindItem(column, row);
            auto& slot = state.inventory[index];
            slot.visible = true;
            slot.enabled = true;
            slot.occupied = item != nullptr;
            slot.selected =
                pointed == control->GetIndex(column, row);

            // iconFrame is a CSS-decoration selector in RmlMuSlot, not the
            // item's texture/type. Leave it neutral; actual item imagery is a
            // separate renderer/asset bridge.
            slot.iconFrame = 0;
        }
    }

    if (CharacterMachine)
    {
        const int pointedEquipment =
            g_pMyInventory->GetPointedItemIndex();

        const std::size_t count = std::min<std::size_t>(
            state.equipment.size(),
            static_cast<std::size_t>(MAX_EQUIPMENT_INDEX));

        for (std::size_t index = 0; index < count; ++index)
        {
            const ITEM& item =
                CharacterMachine->Equipment[index];
            auto& slot = state.equipment[index];
            slot.visible = true;
            slot.enabled = true;
            slot.occupied = item.Type >= 0;
            slot.selected =
                pointedEquipment == static_cast<int>(index);
            slot.iconFrame = 0;
        }

        for (std::size_t index = count;
             index < state.equipment.size();
             ++index)
        {
            state.equipment[index].visible = false;
            state.equipment[index].enabled = false;
        }
    }

    return state;
}

} // namespace

bool RmlInventoryLegacyBridge::Synchronize(
    RmlInventoryPanel& panel) const
{
    if (!panel.IsLoaded())
        return false;

    return panel.ApplyState(BuildState());
}

} // namespace UI::Modern::PC::Inventory

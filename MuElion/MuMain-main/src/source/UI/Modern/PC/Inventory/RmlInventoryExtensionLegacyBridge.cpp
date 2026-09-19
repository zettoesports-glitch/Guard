#include "stdafx.h"
#include "UI/Modern/PC/Inventory/RmlInventoryExtensionLegacyBridge.h"

#include "UI/Modern/PC/Inventory/RmlInventoryExtensionPanel.h"

#include "Engine/Object/ZzzInfomation.h"
#include "Render/Textures/ZzzOpenglUtil.h"
#include "UI/NewUI/Inventory/NewUIInventoryExtension.h"
#include "UI/NewUI/NewUISystem.h"

#include <algorithm>
#include <cstddef>

namespace UI::Modern::PC::Inventory
{

namespace
{

RmlInventoryExtensionPanel::State BuildState()
{
    RmlInventoryExtensionPanel::State state;
    state.viewportWidth = static_cast<int>(WindowWidth);
    state.viewportHeight = static_cast<int>(WindowHeight);

    if (!g_pNewUISystem || !g_pMyInventoryExt || !CharacterAttribute)
        return state;

    state.visible =
        g_pNewUISystem->IsVisible(
            SEASON3B::INTERFACE_INVENTORY_EXT);

    state.openedBags = std::clamp<std::size_t>(
        static_cast<std::size_t>(
            CharacterAttribute->InventoryExtensions),
        0u,
        RmlInventoryExtensionPanel::kBagCount);

    const std::size_t visibleSlots =
        state.openedBags *
        RmlInventoryExtensionPanel::kSlotsPerBag;

    for (std::size_t index = 0;
         index < state.slots.size();
         ++index)
    {
        auto& slot = state.slots[index];
        slot.visible = index < visibleSlots;
        slot.enabled = slot.visible;
        slot.selected = false;
        slot.iconFrame = 0;

        if (!slot.visible)
        {
            slot.occupied = false;
            continue;
        }

        const int legacyIndex =
            MAX_MY_INVENTORY_INDEX +
            static_cast<int>(index);
        slot.occupied =
            g_pMyInventoryExt->FindItem(legacyIndex) != nullptr;
    }

    return state;
}

} // namespace

bool RmlInventoryExtensionLegacyBridge::Synchronize(
    RmlInventoryExtensionPanel& panel) const
{
    if (!panel.IsLoaded())
        return false;

    return panel.ApplyState(BuildState());
}

} // namespace UI::Modern::PC::Inventory

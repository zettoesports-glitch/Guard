#pragma once

namespace UI::Modern::PC::Inventory
{

class RmlInventoryPanel;

// Read-only projection of the legacy inventory controls/equipment array into
// the reconstructed RmlUi inventory state.
class RmlInventoryLegacyBridge
{
public:
    [[nodiscard]] bool Synchronize(RmlInventoryPanel& panel) const;
};

} // namespace UI::Modern::PC::Inventory

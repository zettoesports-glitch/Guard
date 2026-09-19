#pragma once

namespace UI::Modern::PC::Inventory
{

class RmlInventoryExtensionPanel;

class RmlInventoryExtensionLegacyBridge
{
public:
    [[nodiscard]] bool Synchronize(
        RmlInventoryExtensionPanel& panel) const;
};

} // namespace UI::Modern::PC::Inventory

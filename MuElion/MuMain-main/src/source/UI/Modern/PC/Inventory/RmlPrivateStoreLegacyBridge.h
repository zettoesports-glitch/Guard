#pragma once

namespace UI::Modern::PC::Inventory
{

class RmlPrivateStorePanel;

class RmlPrivateStoreLegacyBridge
{
public:
    [[nodiscard]] bool Synchronize(
        RmlPrivateStorePanel& panel) const;
};

} // namespace UI::Modern::PC::Inventory

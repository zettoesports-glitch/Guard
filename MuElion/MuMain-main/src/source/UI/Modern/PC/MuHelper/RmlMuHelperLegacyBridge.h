#pragma once

namespace UI::Modern::PC::MuHelper
{

class RmlMuHelperPanel;

// Read-only adapter from MUHelper::ConfigData / legacy visibility into the
// reconstructed RmlUi presentation. Save/network ownership remains legacy.
class RmlMuHelperLegacyBridge
{
public:
    [[nodiscard]] bool Synchronize(RmlMuHelperPanel& panel) const;
};

} // namespace UI::Modern::PC::MuHelper

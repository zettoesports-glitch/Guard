#pragma once

namespace UI::Modern::PC::HUD
{

class RmlMainFrameLayer;

// Read-only state bridge from the proven legacy HUD/game state into the
// reconstructed RmlUi main-frame presentation. It never loads the panel and
// never mutates legacy input/gameplay state.
class RmlMainFrameLegacyBridge
{
public:
    [[nodiscard]] bool Synchronize(RmlMainFrameLayer& panel) const;
};

} // namespace UI::Modern::PC::HUD

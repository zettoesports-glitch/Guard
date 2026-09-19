#pragma once
namespace SEASON3B { class CNewUIDuelWatchWindow; }
namespace UI::Modern::PC::Combat
{
class RmlDuelWatchPanel;
class RmlDuelWatchLegacyBridge
{
public:
    [[nodiscard]] bool Synchronize(
        const SEASON3B::CNewUIDuelWatchWindow& legacy,
        RmlDuelWatchPanel& modern);
};
}

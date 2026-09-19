#pragma once
namespace SEASON3B { class CNewUIWindowMenu; }
namespace UI::Modern::PC::SystemMenu
{
class RmlWindowMenuPanel;
class RmlWindowMenuLegacyBridge
{
public:
    [[nodiscard]] bool Synchronize(
        const SEASON3B::CNewUIWindowMenu& legacy,
        RmlWindowMenuPanel& modern);
};
}

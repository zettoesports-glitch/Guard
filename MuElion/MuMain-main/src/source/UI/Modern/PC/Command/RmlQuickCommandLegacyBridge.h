#pragma once

namespace SEASON3B { class CNewUIQuickCommandWindow; }
namespace UI::Modern::PC::Command
{
class RmlQuickCommandPanel;
class RmlQuickCommandLegacyBridge
{
public:
    [[nodiscard]] bool Synchronize(
        const SEASON3B::CNewUIQuickCommandWindow& legacy,
        RmlQuickCommandPanel& modern);
};
} // namespace UI::Modern::PC::Command

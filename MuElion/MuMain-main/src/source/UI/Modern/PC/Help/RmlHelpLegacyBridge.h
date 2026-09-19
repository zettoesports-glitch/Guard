#pragma once

#include <cstdint>

namespace SEASON3B
{
class CNewUIHelpWindow;
}

namespace UI::Modern::PC::Help
{
class RmlHelpPanel;

class RmlHelpLegacyBridge
{
public:
    [[nodiscard]] bool Synchronize(
        const SEASON3B::CNewUIHelpWindow& legacy,
        RmlHelpPanel& panel,
        int viewportWidth,
        int viewportHeight);

    void Reset() noexcept;

private:
    std::uint64_t lastFingerprint_ = 0;
    bool hasFingerprint_ = false;
};

} // namespace UI::Modern::PC::Help

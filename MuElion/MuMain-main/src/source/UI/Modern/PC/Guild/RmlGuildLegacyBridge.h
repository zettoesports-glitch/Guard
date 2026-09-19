#pragma once

#include <cstdint>

namespace SEASON3B
{
class CNewUIGuildMakeWindow;
class CNewUIGuildInfoWindow;
}

namespace UI::Modern::PC::Guild
{
class RmlGuildCreatePanel;
class RmlGuildInfoPanel;

class RmlGuildLegacyBridge
{
public:
    [[nodiscard]] bool SynchronizeCreate(
        SEASON3B::CNewUIGuildMakeWindow& legacy,
        RmlGuildCreatePanel& panel,
        int viewportWidth,
        int viewportHeight);

    [[nodiscard]] bool SynchronizeInfo(
        const SEASON3B::CNewUIGuildInfoWindow& legacy,
        RmlGuildInfoPanel& panel,
        int viewportWidth,
        int viewportHeight);

    void Reset() noexcept;

private:
    std::uint64_t createFingerprint_ = 0;
    std::uint64_t infoFingerprint_ = 0;
    bool hasCreateFingerprint_ = false;
    bool hasInfoFingerprint_ = false;
};

} // namespace UI::Modern::PC::Guild

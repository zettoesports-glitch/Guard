#pragma once

#include <cstdint>

namespace SEASON3B
{
class CNewUISlideWindow;
}

namespace UI::Modern::PC::Help
{
class RmlLongNoticeLayer;

class RmlLongNoticeLegacyBridge
{
public:
    [[nodiscard]] bool Synchronize(
        const SEASON3B::CNewUISlideWindow& legacy,
        RmlLongNoticeLayer& layer,
        int viewportWidth,
        int viewportHeight);

    void Reset() noexcept;

private:
    std::uint64_t lastFingerprint_ = 0;
    bool hasFingerprint_ = false;
};

} // namespace UI::Modern::PC::Help

#pragma once

#include <cstdint>

namespace SEASON3B
{
class CNewUIGensRanking;
}

namespace UI::Modern::PC::Gens
{
class RmlGensRankingPanel;

class RmlGensRankingLegacyBridge
{
public:
    [[nodiscard]] bool Synchronize(
        const SEASON3B::CNewUIGensRanking& legacy,
        RmlGensRankingPanel& panel,
        int viewportWidth,
        int viewportHeight);

    void Reset() noexcept;

private:
    std::uint64_t lastFingerprint_ = 0;
    bool hasFingerprint_ = false;
};

} // namespace UI::Modern::PC::Gens

#pragma once

#include <cstdint>

namespace SEASON3B { class CNewUIBuffWindow; }

namespace UI::Modern::PC::HUD
{
class RmlBuffListLayer;

class RmlBuffListLegacyBridge
{
public:
    [[nodiscard]] bool Synchronize(
        SEASON3B::CNewUIBuffWindow& legacy,
        RmlBuffListLayer& layer);
    void Reset() noexcept;

private:
    std::uint64_t fingerprint_ = 0;
    bool hasFingerprint_ = false;
};

} // namespace UI::Modern::PC::HUD

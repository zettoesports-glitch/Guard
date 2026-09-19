#pragma once

#include <cstdint>

class CCharMakeWin;

namespace UI::Modern::PC::Character
{
class RmlCharacterCreatePanel;

class RmlCharacterCreateLegacyBridge
{
public:
    [[nodiscard]] bool Synchronize(
        const CCharMakeWin& legacy,
        RmlCharacterCreatePanel& panel);

    void Reset() noexcept;

private:
    std::uint64_t fingerprint_ = 0;
    bool hasFingerprint_ = false;
};

} // namespace UI::Modern::PC::Character

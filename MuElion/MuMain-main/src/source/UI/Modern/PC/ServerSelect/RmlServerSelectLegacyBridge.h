#pragma once

#include <cstdint>

namespace UI::Modern::PC::ServerSelect
{
class RmlServerSelectPanel;

class RmlServerSelectLegacyBridge
{
public:
    [[nodiscard]] bool Synchronize(RmlServerSelectPanel& panel);
    void Reset() noexcept;

private:
    std::uint64_t fingerprint_ = 0;
    bool hasFingerprint_ = false;
};

} // namespace UI::Modern::PC::ServerSelect

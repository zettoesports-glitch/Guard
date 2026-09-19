#pragma once

#include <cstdint>

namespace SEASON3B
{
class CNewUIMiniMap;
}

namespace UI::Modern::PC::HUD
{
class RmlMiniMapLayer;

class RmlMiniMapLegacyBridge
{
public:
    [[nodiscard]] bool Synchronize(
        const SEASON3B::CNewUIMiniMap& legacy,
        RmlMiniMapLayer& modern);

    void Reset() noexcept;

private:
    bool imageBound_ = false;
    std::uint32_t lastTextureId_ = 0;
    std::uint32_t lastTextureWidth_ = 0;
    std::uint32_t lastTextureHeight_ = 0;
};

} // namespace UI::Modern::PC::HUD

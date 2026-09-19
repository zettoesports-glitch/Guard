#include "stdafx.h"
#include "UI/Modern/PC/HUD/RmlMiniMapLegacyBridge.h"

#include "UI/Modern/PC/HUD/RmlMiniMapLayer.h"
#include "UI/NewUI/HUD/NewUIMiniMap.h"
#include "client/render/LogicalRenderAssetTable.h"

#include <RmlUi/Core/StringUtilities.h>

#include <cstddef>
#include <cstdint>
#include <string>
#include <utility>

namespace UI::Modern::PC::HUD
{

namespace
{
constexpr mu::pipeline::LogicalRenderAssetRef kMiniMapAsset{
    0x4d554d494e494d41ull, 1ull
};

std::string WideToUtf8(const std::wstring& input)
{
    std::string result;
    result.reserve(input.size());

    for (std::size_t index = 0; index < input.size(); ++index)
    {
        char32_t codePoint = static_cast<char32_t>(input[index]);
        if constexpr (sizeof(wchar_t) == 2)
        {
            const std::uint16_t first =
                static_cast<std::uint16_t>(input[index]);
            if (first >= 0xD800u && first <= 0xDBFFu &&
                index + 1u < input.size())
            {
                const std::uint16_t second =
                    static_cast<std::uint16_t>(input[index + 1u]);
                if (second >= 0xDC00u && second <= 0xDFFFu)
                {
                    codePoint = 0x10000u +
                        ((static_cast<char32_t>(first - 0xD800u) << 10u) |
                         static_cast<char32_t>(second - 0xDC00u));
                    ++index;
                }
            }
        }
        result += Rml::StringUtilities::ToUTF8(
            static_cast<Rml::Character>(codePoint));
    }

    return result;
}
} // namespace

bool RmlMiniMapLegacyBridge::Synchronize(
    const SEASON3B::CNewUIMiniMap& legacy,
    RmlMiniMapLayer& modern)
{
    SEASON3B::MiniMapSnapshot snapshot;
    legacy.BuildSnapshot(snapshot);

    bool changed = false;
    if (snapshot.textureReady)
    {
        const bool textureChanged =
            !imageBound_ ||
            lastTextureId_ != snapshot.textureId ||
            lastTextureWidth_ != snapshot.textureWidth ||
            lastTextureHeight_ != snapshot.textureHeight;

        if (textureChanged)
        {
            auto& assets = mu::pipeline::GetLogicalRenderAssetTable();
            if (assets.RegisterBorrowedTexture(
                    kMiniMapAsset,
                    snapshot.textureId,
                    snapshot.textureWidth,
                    snapshot.textureHeight,
                    mu::pipeline::RenderAssetRetention::Persistent,
                    {.linear = true, .repeatU = false, .repeatV = false}))
            {
                if (const auto metadata = assets.Resolve(kMiniMapAsset))
                {
                    imageBound_ = modern.SetMapImage(*metadata);
                    lastTextureId_ = snapshot.textureId;
                    lastTextureWidth_ = snapshot.textureWidth;
                    lastTextureHeight_ = snapshot.textureHeight;
                    changed |= imageBound_;
                }
            }
        }
    }
    else
    {
        imageBound_ = false;
    }

    RmlMiniMapLayer::State state;
    state.visible = snapshot.visible && snapshot.textureReady;
    state.viewportWidth = snapshot.viewportWidth;
    state.viewportHeight = snapshot.viewportHeight;
    state.heroWorldX = static_cast<float>(snapshot.heroX);
    state.heroWorldY = static_cast<float>(snapshot.heroY);
    state.mapWidth = snapshot.mapWidth;
    state.mapHeight = snapshot.mapHeight;
    state.alphaPercent = 55.0f;
    state.markers.reserve(snapshot.markers.size());

    for (const auto& marker : snapshot.markers)
    {
        if (marker.kind != 1 && marker.kind != 2)
            continue;

        RmlMiniMapLayer::Marker out;
        out.kind = marker.kind == 2
            ? RmlMiniMapLayer::MarkerKind::Portal
            : RmlMiniMapLayer::MarkerKind::Npc;
        out.worldX = static_cast<float>(marker.x);
        out.worldY = static_cast<float>(marker.y);
        out.rotationDegrees = static_cast<float>(marker.rotation);
        out.name = WideToUtf8(marker.name);
        state.markers.push_back(std::move(out));
    }

    changed |= modern.ApplyState(std::move(state));
    return changed;
}

void RmlMiniMapLegacyBridge::Reset() noexcept
{
    imageBound_ = false;
    lastTextureId_ = 0;
    lastTextureWidth_ = 0;
    lastTextureHeight_ = 0;
}

} // namespace UI::Modern::PC::HUD

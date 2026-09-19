#pragma once

#include "client/render/LogicalRenderAssetTable.h"

#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace UI::Modern::PC::HUD
{

class RmlMiniMapLayer
{
public:
    enum class MarkerKind
    {
        Npc,
        Portal,
    };

    struct Marker
    {
        MarkerKind kind = MarkerKind::Npc;
        float worldX = 0.0f;
        float worldY = 0.0f;
        float rotationDegrees = 0.0f;
        std::string name;
    };

    struct State
    {
        bool visible = false;
        int viewportWidth = 0;
        int viewportHeight = 0;

        float heroWorldX = 0.0f;
        float heroWorldY = 0.0f;
        float mapWidth = 800.0f;
        float mapHeight = 800.0f;
        float alphaPercent = 55.0f;

        std::vector<Marker> markers;
    };

    RmlMiniMapLayer();
    ~RmlMiniMapLayer();

    RmlMiniMapLayer(const RmlMiniMapLayer&) = delete;
    RmlMiniMapLayer& operator=(const RmlMiniMapLayer&) = delete;
    RmlMiniMapLayer(RmlMiniMapLayer&&) noexcept;
    RmlMiniMapLayer& operator=(RmlMiniMapLayer&&) noexcept;

    [[nodiscard]] bool Load(bool show = false);
    [[nodiscard]] bool Show();
    [[nodiscard]] bool Hide();
    void Release();

    [[nodiscard]] bool IsLoaded() const noexcept;
    [[nodiscard]] bool SetMapImage(
        const mu::pipeline::LogicalRenderAssetMetadata& metadata);
    [[nodiscard]] bool ApplyState(State state);

    // Semantic input boundary. The game input layer may bind the published
    // [, ], -, + controls without RmlUi owning keyboard/network behavior.
    void RequestAlphaStep(int direction) noexcept;
    void RequestScaleStep(int direction) noexcept;
    void RequestClose() noexcept;
    [[nodiscard]] std::optional<int> ConsumeAlphaStep() noexcept;
    [[nodiscard]] std::optional<int> ConsumeScaleStep() noexcept;
    [[nodiscard]] bool ConsumeCloseRequest() noexcept;

private:
    class Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace UI::Modern::PC::HUD

#pragma once

#include "client/render/LogicalRenderAssetTable.h"

#include <RmlUi/Core/CallbackTexture.h>
#include <RmlUi/Core/Element.h>
#include <RmlUi/Core/Geometry.h>

#include <optional>

namespace UI::Modern
{

class RmlHudMapViewport final : public Rml::Element
{
public:
    explicit RmlHudMapViewport(const Rml::String& tag);
    ~RmlHudMapViewport() override = default;

    [[nodiscard]] bool SetImage(
        const mu::pipeline::LogicalRenderAssetMetadata& metadata);

protected:
    void OnRender() override;
    void OnResize() override;

private:
    void RebuildGeometry();

    std::optional<Rml::CallbackTextureSource> m_textureSource;
    Rml::Geometry m_geometry;
    Rml::Vector2i m_imageDimensions{};
};

} // namespace UI::Modern

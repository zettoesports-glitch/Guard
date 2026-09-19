#include "stdafx.h"
#include "UI/Modern/RmlHudMapViewport.h"

#include "UI/Modern/TapeRenderInterface.h"

#include <RmlUi/Core/RenderManager.h>

#include <utility>

namespace UI::Modern
{

RmlHudMapViewport::RmlHudMapViewport(const Rml::String& tag)
    : Rml::Element(tag)
{
}

bool RmlHudMapViewport::SetImage(
    const mu::pipeline::LogicalRenderAssetMetadata& metadata)
{
    if (!metadata.ref.IsValid() || metadata.textureId == 0 ||
        metadata.width == 0 || metadata.height == 0)
        return false;

    const Rml::TextureHandle handle =
        GetTapeRenderInterface().ImportLogicalTexture(metadata);
    if (handle == 0)
        return false;

    m_imageDimensions = {
        static_cast<int>(metadata.width),
        static_cast<int>(metadata.height)
    };

    m_textureSource.emplace(
        [handle, dimensions = m_imageDimensions](
            const Rml::CallbackTextureInterface& textureInterface) -> bool {
            textureInterface.SetTextureHandle(handle, dimensions);
            return true;
        });

    RebuildGeometry();
    DirtyLayout();
    return true;
}

void RmlHudMapViewport::RebuildGeometry()
{
    Rml::RenderManager* renderManager = GetRenderManager();
    if (!renderManager)
        return;

    const float width = GetClientWidth();
    const float height = GetClientHeight();
    if (!(width > 0.0f) || !(height > 0.0f))
        return;

    Rml::Mesh mesh;
    mesh.vertices = {
        {{0.0f, 0.0f},       {255, 255, 255, 255}, {0.0f, 0.0f}},
        {{width, 0.0f},       {255, 255, 255, 255}, {1.0f, 0.0f}},
        {{width, height},     {255, 255, 255, 255}, {1.0f, 1.0f}},
        {{0.0f, height},      {255, 255, 255, 255}, {0.0f, 1.0f}},
    };
    mesh.indices = {0, 1, 2, 0, 2, 3};

    m_geometry = renderManager->MakeGeometry(std::move(mesh));
}

void RmlHudMapViewport::OnResize()
{
    RebuildGeometry();
}

void RmlHudMapViewport::OnRender()
{
    if (!m_textureSource || !m_geometry)
        return;

    Rml::RenderManager* renderManager = GetRenderManager();
    if (!renderManager)
        return;

    const Rml::Texture texture = m_textureSource->GetTexture(*renderManager);
    m_geometry.Render(GetAbsoluteOffset(Rml::BoxArea::Content), texture);
}

} // namespace UI::Modern

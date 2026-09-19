#pragma once

#include "Render/Renderer/MuRenderer.h"
#include "client/render/RenderTapeTypes.h"

#include <cstdint>
#include <memory>
#include <span>
#include <utility>
#include <vector>

namespace mu::pipeline
{

struct LogicalGeometryAssetData
{
    LogicalRenderAssetRef ref{};
    std::uint32_t textureId = 0;
    std::vector<mu::Vertex3D> vertices;
    std::vector<mu::SkinnedVertex3D> skinnedVertices;
    std::vector<std::uint32_t> indices;
};

class LogicalGeometryAssetLease
{
public:
    LogicalGeometryAssetLease() = default;
    explicit LogicalGeometryAssetLease(std::shared_ptr<const LogicalGeometryAssetData> data) noexcept
        : m_data(std::move(data))
    {
    }

    [[nodiscard]] bool IsValid() const noexcept
    {
        return m_data && m_data->ref.IsValid() &&
               (!m_data->vertices.empty() || !m_data->skinnedVertices.empty());
    }

    [[nodiscard]] LogicalRenderAssetRef Ref() const noexcept
    {
        return m_data ? m_data->ref : LogicalRenderAssetRef{};
    }

    [[nodiscard]] std::uint32_t TextureId() const noexcept
    {
        return m_data ? m_data->textureId : 0u;
    }

    [[nodiscard]] std::span<const mu::Vertex3D> Vertices() const noexcept
    {
        return m_data ? std::span<const mu::Vertex3D>(m_data->vertices) : std::span<const mu::Vertex3D>{};
    }

    [[nodiscard]] std::span<const mu::SkinnedVertex3D> SkinnedVertices() const noexcept
    {
        return m_data ? std::span<const mu::SkinnedVertex3D>(m_data->skinnedVertices)
                      : std::span<const mu::SkinnedVertex3D>{};
    }

    [[nodiscard]] std::span<const std::uint32_t> Indices() const noexcept
    {
        return m_data ? std::span<const std::uint32_t>(m_data->indices)
                      : std::span<const std::uint32_t>{};
    }

private:
    std::shared_ptr<const LogicalGeometryAssetData> m_data;
};

struct TrustedGeometryDraw
{
    LogicalGeometryAssetLease geometry;
    std::uint32_t first = 0;
    std::uint32_t count = 0;
};

[[nodiscard]] inline LogicalGeometryAssetLease MakeLogicalGeometryAssetLease(
    LogicalGeometryAssetData data)
{
    return LogicalGeometryAssetLease(
        std::make_shared<const LogicalGeometryAssetData>(std::move(data)));
}

} // namespace mu::pipeline

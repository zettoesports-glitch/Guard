#pragma once

#include "client/render/LegacyRenderFacade.h"
#include "client/render/LogicalRenderAssetTable.h"

#include <RmlUi/Core/RenderInterface.h>

#include <cstdint>
#include <memory>
#include <mutex>
#include <optional>
#include <unordered_map>
#include <vector>

namespace UI::Modern
{

class TapeRenderInterface final : public Rml::RenderInterface
{
public:
    TapeRenderInterface(mu::pipeline::LegacyRenderFacade& facade,
                        mu::pipeline::LogicalRenderAssetTable& assets) noexcept;
    ~TapeRenderInterface() override;

    Rml::CompiledGeometryHandle CompileGeometry(
        Rml::Span<const Rml::Vertex> vertices,
        Rml::Span<const int> indices) override;
    void RenderGeometry(Rml::CompiledGeometryHandle geometry,
                        Rml::Vector2f translation,
                        Rml::TextureHandle texture) override;
    void ReleaseGeometry(Rml::CompiledGeometryHandle geometry) override;

    Rml::TextureHandle LoadTexture(Rml::Vector2i& textureDimensions,
                                   const Rml::String& source) override;
    Rml::TextureHandle GenerateTexture(Rml::Span<const Rml::byte> source,
                                       Rml::Vector2i sourceDimensions) override;
    void ReleaseTexture(Rml::TextureHandle texture) override;

    void EnableScissorRegion(bool enable) override;
    void SetScissorRegion(Rml::Rectanglei region) override;
    void SetTransform(const Rml::Matrix4f* transform) override;

    [[nodiscard]] std::size_t GeometryCount() const noexcept;
    [[nodiscard]] std::size_t TextureCount() const noexcept;

private:
    struct CompiledGeometry
    {
        std::vector<Rml::Vertex> vertices;
        std::vector<int> indices;
    };

public:
    // Kept public because the private Debug RTTI exposes this exact nested type.
    struct LoadedTexture
    {
        mu::pipeline::LogicalRenderAssetRef asset{};
        std::uint32_t width = 0;
        std::uint32_t height = 0;
    };

private:
    [[nodiscard]] Rml::CompiledGeometryHandle AllocateGeometryHandle() noexcept;
    [[nodiscard]] Rml::TextureHandle AllocateTextureHandle() noexcept;
    [[nodiscard]] static std::uint32_t PackColor(
        const Rml::ColourbPremultiplied& color) noexcept;
    [[nodiscard]] static bool DecodeTga(
        Rml::Span<const Rml::byte> bytes,
        int& width, int& height,
        std::vector<Rml::byte>& premultipliedRgba) noexcept;
    [[nodiscard]] static bool ReadRmlFile(
        const Rml::String& source,
        std::vector<Rml::byte>& bytes) noexcept;

    mu::pipeline::LegacyRenderFacade& m_facade;
    mu::pipeline::LogicalRenderAssetTable& m_assets;

    mutable std::mutex m_mutex;
    std::unordered_map<Rml::CompiledGeometryHandle,
                       std::shared_ptr<CompiledGeometry>> m_geometries;
    std::unordered_map<Rml::TextureHandle,
                       std::shared_ptr<LoadedTexture>> m_textures;

    std::uintptr_t m_nextGeometryHandle = 1;
    std::uintptr_t m_nextTextureHandle = 1;
    std::optional<Rml::Matrix4f> m_transform;
};

TapeRenderInterface& GetTapeRenderInterface();

} // namespace UI::Modern

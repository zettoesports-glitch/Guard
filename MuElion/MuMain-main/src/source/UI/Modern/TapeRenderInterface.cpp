#include "stdafx.h"
#include "UI/Modern/TapeRenderInterface.h"

#include <RmlUi/Core/Core.h>
#include <RmlUi/Core/FileInterface.h>

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <cstring>
#include <limits>
#include <span>

namespace UI::Modern
{

namespace
{
constexpr std::uint64_t kRmlLogicalAssetPrefix = 0x524D4C0000000000ull;

bool CheckedPixelBytes(int width, int height, std::size_t& bytes) noexcept
{
    if (width <= 0 || height <= 0)
        return false;

    const std::size_t w = static_cast<std::size_t>(width);
    const std::size_t h = static_cast<std::size_t>(height);
    if (w > std::numeric_limits<std::size_t>::max() / h)
        return false;
    const std::size_t pixels = w * h;
    if (pixels > std::numeric_limits<std::size_t>::max() / 4u)
        return false;
    bytes = pixels * 4u;
    return true;
}

std::uint16_t ReadLe16(const Rml::byte* data) noexcept
{
    return static_cast<std::uint16_t>(
        static_cast<std::uint16_t>(data[0]) |
        (static_cast<std::uint16_t>(data[1]) << 8u));
}
} // namespace

TapeRenderInterface::TapeRenderInterface(
    mu::pipeline::LegacyRenderFacade& facade,
    mu::pipeline::LogicalRenderAssetTable& assets) noexcept
    : m_facade(facade), m_assets(assets)
{
}

TapeRenderInterface::~TapeRenderInterface()
{
    std::vector<mu::pipeline::LogicalRenderAssetRef> assets;
    {
        std::scoped_lock lock(m_mutex);
        assets.reserve(m_textures.size());
        for (const auto& [_, texture] : m_textures)
            if (texture)
                assets.push_back(texture->asset);
        m_textures.clear();
        m_geometries.clear();
    }

    for (const auto ref : assets)
        m_assets.Release(ref);
}

Rml::CompiledGeometryHandle TapeRenderInterface::AllocateGeometryHandle() noexcept
{
    for (;;)
    {
        const auto value = m_nextGeometryHandle++;
        if (value != 0 && !m_geometries.contains(value))
            return value;
    }
}

Rml::TextureHandle TapeRenderInterface::AllocateTextureHandle() noexcept
{
    for (;;)
    {
        const auto value = m_nextTextureHandle++;
        if (value != 0 && !m_textures.contains(value))
            return value;
    }
}

Rml::CompiledGeometryHandle TapeRenderInterface::CompileGeometry(
    Rml::Span<const Rml::Vertex> vertices,
    Rml::Span<const int> indices)
{
    if (vertices.empty() || indices.empty() || indices.size() % 3u != 0u)
        return 0;

    for (const int index : indices)
    {
        if (index < 0 || static_cast<std::size_t>(index) >= vertices.size())
            return 0;
    }

    auto geometry = std::make_shared<CompiledGeometry>();
    geometry->vertices.assign(vertices.begin(), vertices.end());
    geometry->indices.assign(indices.begin(), indices.end());

    std::scoped_lock lock(m_mutex);
    const auto handle = AllocateGeometryHandle();
    m_geometries.emplace(handle, std::move(geometry));
    return handle;
}

std::uint32_t TapeRenderInterface::PackColor(
    const Rml::ColourbPremultiplied& color) noexcept
{
    return (static_cast<std::uint32_t>(color.alpha) << 24u) |
           (static_cast<std::uint32_t>(color.blue) << 16u) |
           (static_cast<std::uint32_t>(color.green) << 8u) |
           static_cast<std::uint32_t>(color.red);
}

void TapeRenderInterface::RenderGeometry(
    Rml::CompiledGeometryHandle geometryHandle,
    Rml::Vector2f translation,
    Rml::TextureHandle textureHandle)
{
    std::shared_ptr<CompiledGeometry> geometry;
    std::shared_ptr<LoadedTexture> texture;
    std::optional<Rml::Matrix4f> transform;

    {
        std::scoped_lock lock(m_mutex);
        const auto geometryIt = m_geometries.find(geometryHandle);
        if (geometryIt == m_geometries.end())
            return;
        geometry = geometryIt->second;

        if (textureHandle != 0)
        {
            const auto textureIt = m_textures.find(textureHandle);
            if (textureIt == m_textures.end())
                return;
            texture = textureIt->second;
        }
        transform = m_transform;
    }

    std::uint32_t textureId = 0;
    if (texture)
    {
        const auto metadata = m_assets.Resolve(texture->asset);
        if (!metadata)
            return;
        textureId = metadata->textureId;
    }

    std::vector<mu::Vertex2D> triangles;
    triangles.reserve(geometry->indices.size());

    for (const int index : geometry->indices)
    {
        const auto& source = geometry->vertices[static_cast<std::size_t>(index)];
        float x = source.position.x + translation.x;
        float y = source.position.y + translation.y;

        if (transform)
        {
            const Rml::Vector4f input(x, y, 0.0f, 1.0f);
            const Rml::Vector4f output = (*transform) * input;
            if (std::abs(output.w) > 1.0e-7f)
            {
                x = output.x / output.w;
                y = output.y / output.w;
            }
            else
            {
                x = output.x;
                y = output.y;
            }
        }

        triangles.push_back({
            x, y,
            source.tex_coord.x, source.tex_coord.y,
            PackColor(source.colour)
        });
    }

    // RmlUi vertices and generated textures are premultiplied-alpha.
    // MuMain's BlendMode::Mixed maps to ONE / ONE_MINUS_SRC_ALPHA, matching
    // RmlUi's official SDL GPU backend.
    (void)m_facade.PushAttrib();
    (void)m_facade.SetDepthTestEnable(false);
    (void)m_facade.SetDepthWriteEnable(false);
    (void)m_facade.SetCullEnable(false);
    (void)m_facade.SetFogEnable(false);
    (void)m_facade.SetBlendEnable(true);
    (void)m_facade.SetBlendFunc(
        mu::pipeline::RenderBlendFactor::One,
        mu::pipeline::RenderBlendFactor::OneMinusSourceAlpha);
    (void)m_facade.SetTextureEnable(textureId != 0);
    (void)m_facade.SubmitTriangles2D(triangles, textureId);
    (void)m_facade.PopAttrib();
}

void TapeRenderInterface::ReleaseGeometry(Rml::CompiledGeometryHandle geometry)
{
    std::scoped_lock lock(m_mutex);
    m_geometries.erase(geometry);
}

bool TapeRenderInterface::ReadRmlFile(
    const Rml::String& source,
    std::vector<Rml::byte>& bytes) noexcept
{
    Rml::FileInterface* files = Rml::GetFileInterface();
    if (!files)
        return false;

    const Rml::FileHandle file = files->Open(source);
    if (!file)
        return false;

    const std::size_t size = files->Length(file);
    if (size == 0)
    {
        files->Close(file);
        return false;
    }

    bytes.resize(size);
    const std::size_t read = files->Read(bytes.data(), bytes.size(), file);
    files->Close(file);
    return read == bytes.size();
}

bool TapeRenderInterface::DecodeTga(
    Rml::Span<const Rml::byte> bytes,
    int& width, int& height,
    std::vector<Rml::byte>& rgba) noexcept
{
    if (bytes.size() < 18)
        return false;

    const auto* header = bytes.data();
    const std::uint8_t idLength = header[0];
    const std::uint8_t colorMapType = header[1];
    const std::uint8_t imageType = header[2];
    const int imageWidth = ReadLe16(header + 12);
    const int imageHeight = ReadLe16(header + 14);
    const std::uint8_t bitsPerPixel = header[16];
    const std::uint8_t descriptor = header[17];

    if (colorMapType != 0 || (imageType != 2 && imageType != 10) ||
        (bitsPerPixel != 24 && bitsPerPixel != 32))
        return false;

    std::size_t byteCount = 0;
    if (!CheckedPixelBytes(imageWidth, imageHeight, byteCount))
        return false;

    const std::size_t sourcePixelBytes = bitsPerPixel / 8u;
    std::size_t offset = 18u + idLength;
    if (offset > bytes.size())
        return false;

    rgba.assign(byteCount, 0);
    const bool topOrigin = (descriptor & 0x20u) != 0;
    const bool rightOrigin = (descriptor & 0x10u) != 0;
    const std::size_t pixelCount =
        static_cast<std::size_t>(imageWidth) * static_cast<std::size_t>(imageHeight);

    auto writePixel = [&](std::size_t filePixel, const Rml::byte* source) {
        const std::size_t fileX = filePixel % static_cast<std::size_t>(imageWidth);
        const std::size_t fileY = filePixel / static_cast<std::size_t>(imageWidth);
        const std::size_t x = rightOrigin ?
            static_cast<std::size_t>(imageWidth - 1) - fileX : fileX;
        const std::size_t y = topOrigin ?
            fileY : static_cast<std::size_t>(imageHeight - 1) - fileY;
        const std::size_t destination =
            (y * static_cast<std::size_t>(imageWidth) + x) * 4u;

        const std::uint32_t alpha = sourcePixelBytes == 4u ? source[3] : 255u;
        rgba[destination + 0] =
            static_cast<Rml::byte>((static_cast<std::uint32_t>(source[2]) * alpha) / 255u);
        rgba[destination + 1] =
            static_cast<Rml::byte>((static_cast<std::uint32_t>(source[1]) * alpha) / 255u);
        rgba[destination + 2] =
            static_cast<Rml::byte>((static_cast<std::uint32_t>(source[0]) * alpha) / 255u);
        rgba[destination + 3] = static_cast<Rml::byte>(alpha);
    };

    if (imageType == 2)
    {
        const std::size_t required = pixelCount * sourcePixelBytes;
        if (offset > bytes.size() || required > bytes.size() - offset)
            return false;
        for (std::size_t pixel = 0; pixel < pixelCount; ++pixel)
            writePixel(pixel, bytes.data() + offset + pixel * sourcePixelBytes);
    }
    else
    {
        std::size_t pixel = 0;
        while (pixel < pixelCount)
        {
            if (offset >= bytes.size())
                return false;
            const std::uint8_t packet = bytes[offset++];
            const std::size_t runLength = static_cast<std::size_t>((packet & 0x7fu) + 1u);
            if (runLength > pixelCount - pixel)
                return false;

            if ((packet & 0x80u) != 0)
            {
                if (offset > bytes.size() ||
                    sourcePixelBytes > bytes.size() - offset)
                    return false;
                const auto* source = bytes.data() + offset;
                offset += sourcePixelBytes;
                for (std::size_t i = 0; i < runLength; ++i)
                    writePixel(pixel++, source);
            }
            else
            {
                const std::size_t packetBytes = runLength * sourcePixelBytes;
                if (offset > bytes.size() || packetBytes > bytes.size() - offset)
                    return false;
                for (std::size_t i = 0; i < runLength; ++i)
                {
                    writePixel(pixel++, bytes.data() + offset);
                    offset += sourcePixelBytes;
                }
            }
        }
    }

    width = imageWidth;
    height = imageHeight;
    return true;
}

Rml::TextureHandle TapeRenderInterface::LoadTexture(
    Rml::Vector2i& textureDimensions,
    const Rml::String& source)
{
    std::vector<Rml::byte> bytes;
    if (!ReadRmlFile(source, bytes))
        return 0;

    const auto extensionPosition = source.find_last_of('.');
    if (extensionPosition == Rml::String::npos)
        return 0;

    Rml::String extension = source.substr(extensionPosition + 1);
    std::transform(extension.begin(), extension.end(), extension.begin(),
                   [](char c) { return static_cast<char>(std::tolower(
                       static_cast<unsigned char>(c))); });

    if (extension != "tga")
        return 0;

    int width = 0;
    int height = 0;
    std::vector<Rml::byte> rgba;
    if (!DecodeTga(bytes, width, height, rgba))
        return 0;

    textureDimensions = {width, height};
    return GenerateTexture(rgba, textureDimensions);
}

Rml::TextureHandle TapeRenderInterface::GenerateTexture(
    Rml::Span<const Rml::byte> source,
    Rml::Vector2i sourceDimensions)
{
    std::size_t expectedBytes = 0;
    if (!CheckedPixelBytes(
            sourceDimensions.x, sourceDimensions.y, expectedBytes) ||
        source.size() < expectedBytes)
        return 0;

    Rml::TextureHandle handle = 0;
    mu::pipeline::LogicalRenderAssetRef ref{};
    {
        std::scoped_lock lock(m_mutex);
        handle = AllocateTextureHandle();
        ref = {
            kRmlLogicalAssetPrefix |
                static_cast<std::uint64_t>(handle),
            1u
        };
    }

    const auto pixelBytes = std::span<const std::byte>(
        reinterpret_cast<const std::byte*>(source.data()), expectedBytes);

    mu::pipeline::RenderSamplerIntent sampler{};
    sampler.linear = true;
    sampler.repeatU = false;
    sampler.repeatV = false;

    if (!m_assets.DefineTexture2D(
            ref,
            static_cast<std::uint32_t>(sourceDimensions.x),
            static_cast<std::uint32_t>(sourceDimensions.y),
            pixelBytes,
            mu::pipeline::LegacyPixelFormat::Rgba8,
            mu::pipeline::RenderAssetRetention::Persistent,
            sampler))
        return 0;

    auto texture = std::make_shared<LoadedTexture>();
    texture->asset = ref;
    texture->width = static_cast<std::uint32_t>(sourceDimensions.x);
    texture->height = static_cast<std::uint32_t>(sourceDimensions.y);
    texture->ownsAsset = true;

    {
        std::scoped_lock lock(m_mutex);
        m_textures.emplace(handle, std::move(texture));
    }
    return handle;
}

Rml::TextureHandle TapeRenderInterface::ImportLogicalTexture(
    const mu::pipeline::LogicalRenderAssetMetadata& metadata)
{
    if (!metadata.ref.IsValid() || metadata.textureId == 0 ||
        metadata.width == 0 || metadata.height == 0)
        return 0;

    auto texture = std::make_shared<LoadedTexture>();
    texture->asset = metadata.ref;
    texture->width = metadata.width;
    texture->height = metadata.height;
    texture->ownsAsset = false;

    std::scoped_lock lock(m_mutex);
    const Rml::TextureHandle handle = AllocateTextureHandle();
    m_textures.emplace(handle, std::move(texture));
    return handle;
}

void TapeRenderInterface::ReleaseTexture(Rml::TextureHandle textureHandle)
{
    std::shared_ptr<LoadedTexture> texture;
    {
        std::scoped_lock lock(m_mutex);
        const auto it = m_textures.find(textureHandle);
        if (it == m_textures.end())
            return;
        texture = std::move(it->second);
        m_textures.erase(it);
    }

    if (texture && texture->ownsAsset)
        m_assets.Release(texture->asset);
}

void TapeRenderInterface::EnableScissorRegion(bool enable)
{
    (void)m_facade.SetScissorEnable(enable);
}

void TapeRenderInterface::SetScissorRegion(Rml::Rectanglei region)
{
    (void)m_facade.SetScissor({
        region.Left(), region.Top(),
        region.Width(), region.Height()
    });
}

void TapeRenderInterface::SetTransform(const Rml::Matrix4f* transform)
{
    std::scoped_lock lock(m_mutex);
    if (transform)
        m_transform = *transform;
    else
        m_transform.reset();
}

std::size_t TapeRenderInterface::GeometryCount() const noexcept
{
    std::scoped_lock lock(m_mutex);
    return m_geometries.size();
}

std::size_t TapeRenderInterface::TextureCount() const noexcept
{
    std::scoped_lock lock(m_mutex);
    return m_textures.size();
}

TapeRenderInterface& GetTapeRenderInterface()
{
    static TapeRenderInterface interface(
        mu::pipeline::GetLegacyRenderFacade(),
        mu::pipeline::GetLogicalRenderAssetTable());
    return interface;
}

} // namespace UI::Modern

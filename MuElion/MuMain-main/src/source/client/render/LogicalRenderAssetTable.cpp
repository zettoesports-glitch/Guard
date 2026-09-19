#include "stdafx.h"
#include "client/render/LogicalRenderAssetTable.h"

#include "Render/Renderer/MuRenderer.h"
#include "client/render/FrameTape.h"

#include <algorithm>
#include <limits>

namespace mu::pipeline
{

namespace
{
bool TryPixelBytes(std::uint32_t width, std::uint32_t height, std::size_t channels, std::size_t& bytes) noexcept
{
    if (width == 0 || height == 0)
        return false;

    const std::size_t pixels = static_cast<std::size_t>(width) * static_cast<std::size_t>(height);
    if (pixels > std::numeric_limits<std::size_t>::max() / channels)
        return false;

    bytes = pixels * channels;
    return true;
}
} // namespace

void LogicalRenderAssetTable::ConfigureSharedIdleSeconds(std::uint32_t seconds)
{
    std::scoped_lock lock(m_mutex);
    m_sharedIdle = std::chrono::seconds(seconds);
}

bool LogicalRenderAssetTable::ConvertToRgba8(std::span<const std::byte> pixels,
                                             std::uint32_t width,
                                             std::uint32_t height,
                                             LegacyPixelFormat format,
                                             std::vector<std::uint8_t>& output) noexcept
{
    const std::size_t channels = format == LegacyPixelFormat::Rgb8 ? 3u : 4u;
    std::size_t expected = 0;
    if (!TryPixelBytes(width, height, channels, expected) || pixels.size() < expected)
        return false;

    const auto* src = reinterpret_cast<const std::uint8_t*>(pixels.data());
    const std::size_t pixelCount = static_cast<std::size_t>(width) * static_cast<std::size_t>(height);
    output.resize(pixelCount * 4u);

    for (std::size_t i = 0; i < pixelCount; ++i)
    {
        if (format == LegacyPixelFormat::Rgb8)
        {
            output[i * 4u + 0u] = src[i * 3u + 0u];
            output[i * 4u + 1u] = src[i * 3u + 1u];
            output[i * 4u + 2u] = src[i * 3u + 2u];
            output[i * 4u + 3u] = 255u;
        }
        else if (format == LegacyPixelFormat::Bgra8)
        {
            output[i * 4u + 0u] = src[i * 4u + 2u];
            output[i * 4u + 1u] = src[i * 4u + 1u];
            output[i * 4u + 2u] = src[i * 4u + 0u];
            output[i * 4u + 3u] = src[i * 4u + 3u];
        }
        else
        {
            output[i * 4u + 0u] = src[i * 4u + 0u];
            output[i * 4u + 1u] = src[i * 4u + 1u];
            output[i * 4u + 2u] = src[i * 4u + 2u];
            output[i * 4u + 3u] = src[i * 4u + 3u];
        }
    }

    return true;
}

bool LogicalRenderAssetTable::DefineTexture2D(LogicalRenderAssetRef ref,
                                               std::uint32_t width,
                                               std::uint32_t height,
                                               std::span<const std::byte> pixels,
                                               LegacyPixelFormat format,
                                               RenderAssetRetention retention,
                                               RenderSamplerIntent sampler) noexcept
{
    if (!ref.IsValid() || width == 0 || height == 0)
        return false;

    std::vector<std::uint8_t> rgba;
    if (!ConvertToRgba8(pixels, width, height, format, rgba))
    {
        GetFrameTape().RejectAsset(ref.id, ref.revision, FrameRejectionReason::InvalidTapeContents, 0);
        return false;
    }

    std::scoped_lock lock(m_mutex);
    auto& entry = m_entries[ref.id];

    if (entry.metadata.textureId != 0 && entry.metadata.ref.revision != ref.revision)
    {
        mu::GetRenderer().ReleaseTexture(entry.metadata.textureId);
        entry.metadata.textureId = 0;
    }

    if (entry.metadata.textureId == 0)
    {
        entry.metadata.textureId = mu::GetRenderer().CreateTexture(width, height, rgba.data());
        if (entry.metadata.textureId == 0)
        {
            GetFrameTape().RejectAsset(ref.id, ref.revision, FrameRejectionReason::MissingLogicalAsset, 0);
            m_entries.erase(ref.id);
            return false;
        }
    }
    else
    {
        mu::GetRenderer().EnsureTexture(entry.metadata.textureId, width, height);
        mu::GetRenderer().QueueTextureUpdate(entry.metadata.textureId, rgba.data(), width, height);
    }

    entry.metadata.ref = ref;
    entry.metadata.width = width;
    entry.metadata.height = height;
    entry.metadata.retention = retention;
    entry.metadata.sampler = sampler;
    entry.rgba8 = std::move(rgba);
    entry.lastUsed = std::chrono::steady_clock::now();
    return true;
}

bool LogicalRenderAssetTable::RegisterCapturedTexture(
    LogicalRenderAssetRef ref, std::uint32_t textureId,
    std::uint32_t width, std::uint32_t height,
    RenderAssetRetention retention, RenderSamplerIntent sampler) noexcept
{
    if (!ref.IsValid() || textureId == 0 || width == 0 || height == 0 ||
        !mu::GetRenderer().IsTextureRegistered(textureId))
        return false;

    std::scoped_lock lock(m_mutex);
    auto& entry = m_entries[ref.id];
    if (entry.metadata.textureId != 0 && entry.metadata.textureId != textureId)
        mu::GetRenderer().ReleaseTexture(entry.metadata.textureId);

    entry.metadata.ref = ref;
    entry.metadata.textureId = textureId;
    entry.metadata.width = width;
    entry.metadata.height = height;
    entry.metadata.retention = retention;
    entry.metadata.sampler = sampler;
    entry.rgba8.clear();
    entry.lastUsed = std::chrono::steady_clock::now();
    return true;
}

std::optional<LogicalRenderAssetMetadata> LogicalRenderAssetTable::Resolve(LogicalRenderAssetRef ref) noexcept
{
    if (!ref.IsValid())
        return std::nullopt;

    std::scoped_lock lock(m_mutex);
    const auto it = m_entries.find(ref.id);
    if (it == m_entries.end())
    {
        GetFrameTape().RejectAsset(ref.id, ref.revision, FrameRejectionReason::MissingLogicalAsset, 0);
        return std::nullopt;
    }

    if (it->second.metadata.ref.revision != ref.revision)
    {
        GetFrameTape().RejectAsset(ref.id, ref.revision, FrameRejectionReason::AssetTableMismatch, 0);
        return std::nullopt;
    }

    it->second.lastUsed = std::chrono::steady_clock::now();
    return it->second.metadata;
}

std::optional<LogicalRenderAssetRgba8Snapshot>
LogicalRenderAssetTable::SnapshotRgba8(LogicalRenderAssetRef ref) noexcept
{
    if (!ref.IsValid())
        return std::nullopt;

    std::scoped_lock lock(m_mutex);
    const auto it = m_entries.find(ref.id);
    if (it == m_entries.end() ||
        it->second.metadata.ref.revision != ref.revision ||
        it->second.rgba8.empty())
        return std::nullopt;

    it->second.lastUsed = std::chrono::steady_clock::now();
    return LogicalRenderAssetRgba8Snapshot{
        it->second.metadata,
        it->second.rgba8
    };
}

void LogicalRenderAssetTable::Touch(LogicalRenderAssetRef ref) noexcept
{
    std::scoped_lock lock(m_mutex);
    if (const auto it = m_entries.find(ref.id); it != m_entries.end() && it->second.metadata.ref.revision == ref.revision)
        it->second.lastUsed = std::chrono::steady_clock::now();
}

void LogicalRenderAssetTable::Release(LogicalRenderAssetRef ref) noexcept
{
    std::scoped_lock lock(m_mutex);
    const auto it = m_entries.find(ref.id);
    if (it == m_entries.end() || it->second.metadata.ref.revision != ref.revision)
        return;

    if (it->second.metadata.textureId != 0)
        mu::GetRenderer().ReleaseTexture(it->second.metadata.textureId);
    m_entries.erase(it);
}

void LogicalRenderAssetTable::ReleaseFrameOnly() noexcept
{
    std::scoped_lock lock(m_mutex);
    for (auto it = m_entries.begin(); it != m_entries.end();)
    {
        if (it->second.metadata.retention != RenderAssetRetention::FrameOnly)
        {
            ++it;
            continue;
        }

        if (it->second.metadata.textureId != 0)
            mu::GetRenderer().ReleaseTexture(it->second.metadata.textureId);
        it = m_entries.erase(it);
    }
}

void LogicalRenderAssetTable::CollectIdle() noexcept
{
    const auto now = std::chrono::steady_clock::now();
    std::scoped_lock lock(m_mutex);

    for (auto it = m_entries.begin(); it != m_entries.end();)
    {
        const auto& entry = it->second;
        if (entry.metadata.retention != RenderAssetRetention::Shared ||
            now - entry.lastUsed < m_sharedIdle)
        {
            ++it;
            continue;
        }

        if (entry.metadata.textureId != 0)
            mu::GetRenderer().ReleaseTexture(entry.metadata.textureId);
        it = m_entries.erase(it);
    }
}

void LogicalRenderAssetTable::Clear() noexcept
{
    std::scoped_lock lock(m_mutex);
    for (auto& [_, entry] : m_entries)
    {
        if (entry.metadata.textureId != 0)
            mu::GetRenderer().ReleaseTexture(entry.metadata.textureId);
    }
    m_entries.clear();
}

std::size_t LogicalRenderAssetTable::Size() const noexcept
{
    std::scoped_lock lock(m_mutex);
    return m_entries.size();
}

LogicalRenderAssetTable& GetLogicalRenderAssetTable()
{
    static LogicalRenderAssetTable table;
    return table;
}

} // namespace mu::pipeline

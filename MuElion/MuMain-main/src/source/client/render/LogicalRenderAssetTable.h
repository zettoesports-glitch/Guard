#pragma once

#include "client/render/RenderTapeTypes.h"

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <mutex>
#include <optional>
#include <span>
#include <unordered_map>
#include <vector>

namespace mu::pipeline
{

struct LogicalRenderAssetMetadata
{
    LogicalRenderAssetRef ref{};
    std::uint32_t textureId = 0;
    std::uint32_t width = 0;
    std::uint32_t height = 0;
    RenderAssetRetention retention = RenderAssetRetention::Shared;
    RenderSamplerIntent sampler{};
};

class LogicalRenderAssetTable
{
public:
    void ConfigureSharedIdleSeconds(std::uint32_t seconds);

    [[nodiscard]] bool DefineTexture2D(LogicalRenderAssetRef ref,
                                       std::uint32_t width,
                                       std::uint32_t height,
                                       std::span<const std::byte> pixels,
                                       LegacyPixelFormat format,
                                       RenderAssetRetention retention,
                                       RenderSamplerIntent sampler) noexcept;

    [[nodiscard]] bool RegisterCapturedTexture(
        LogicalRenderAssetRef ref, std::uint32_t textureId,
        std::uint32_t width, std::uint32_t height,
        RenderAssetRetention retention = RenderAssetRetention::Shared,
        RenderSamplerIntent sampler = {}) noexcept;

    [[nodiscard]] std::optional<LogicalRenderAssetMetadata> Resolve(LogicalRenderAssetRef ref) noexcept;
    void Touch(LogicalRenderAssetRef ref) noexcept;
    void Release(LogicalRenderAssetRef ref) noexcept;
    void ReleaseFrameOnly() noexcept;
    void CollectIdle() noexcept;
    void Clear() noexcept;

    [[nodiscard]] std::size_t Size() const noexcept;

private:
    struct Entry
    {
        LogicalRenderAssetMetadata metadata{};
        std::chrono::steady_clock::time_point lastUsed{};
    };

    [[nodiscard]] static bool ConvertToRgba8(std::span<const std::byte> pixels,
                                             std::uint32_t width,
                                             std::uint32_t height,
                                             LegacyPixelFormat format,
                                             std::vector<std::uint8_t>& output) noexcept;

    mutable std::mutex m_mutex;
    std::unordered_map<std::uint64_t, Entry> m_entries;
    std::chrono::seconds m_sharedIdle{1};
};

LogicalRenderAssetTable& GetLogicalRenderAssetTable();

} // namespace mu::pipeline

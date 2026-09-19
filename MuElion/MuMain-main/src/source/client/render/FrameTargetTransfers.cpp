#include "stdafx.h"
#include "client/render/FrameTargetTransfers.h"

#include "Render/Renderer/MuRenderer.h"
#include "client/render/LogicalRenderAssetTable.h"

#include <algorithm>
#include <cstddef>
#include <cstring>
#include <limits>
#include <span>

namespace mu::pipeline
{

bool FrameTargetTransfers::NormalizeRect(const RenderTapeRect& requested,
                                         std::uint32_t frameWidth,
                                         std::uint32_t frameHeight,
                                         RenderTapeRect& normalized) noexcept
{
    if (frameWidth == 0 || frameHeight == 0)
        return false;
    if (requested.width <= 0 || requested.height <= 0)
    {
        normalized = {0, 0, static_cast<int>(frameWidth), static_cast<int>(frameHeight)};
        return true;
    }
    if (requested.x < 0 || requested.y < 0)
        return false;

    const std::uint64_t right = static_cast<std::uint64_t>(requested.x) +
                                static_cast<std::uint64_t>(requested.width);
    const std::uint64_t bottom = static_cast<std::uint64_t>(requested.y) +
                                 static_cast<std::uint64_t>(requested.height);
    if (right > frameWidth || bottom > frameHeight)
        return false;
    normalized = requested;
    return true;
}

bool FrameTargetTransfers::Crop(const mu::FramePixelsRgba8& frame,
                                const RenderTapeRect& rect,
                                bool reverseRows,
                                std::vector<std::uint8_t>& rgba) noexcept
{
    if (frame.rgba.empty() || rect.width <= 0 || rect.height <= 0)
        return false;

    constexpr std::size_t bpp = 4;
    const std::size_t srcRowBytes = static_cast<std::size_t>(frame.width) * bpp;
    const std::size_t dstRowBytes = static_cast<std::size_t>(rect.width) * bpp;
    const std::size_t rows = static_cast<std::size_t>(rect.height);
    if (rows != 0 && dstRowBytes > std::numeric_limits<std::size_t>::max() / rows)
        return false;

    rgba.resize(dstRowBytes * rows);
    for (std::size_t dstRow = 0; dstRow < rows; ++dstRow)
    {
        const std::size_t logicalRow = reverseRows ? rows - dstRow - 1 : dstRow;
        const std::size_t srcRow = static_cast<std::size_t>(rect.y) + logicalRow;
        const std::size_t srcOffset = srcRow * srcRowBytes +
                                      static_cast<std::size_t>(rect.x) * bpp;
        const std::size_t dstOffset = dstRow * dstRowBytes;
        if (srcOffset > frame.rgba.size() ||
            dstRowBytes > frame.rgba.size() - srcOffset)
            return false;
        std::memcpy(rgba.data() + dstOffset, frame.rgba.data() + srcOffset, dstRowBytes);
    }
    return true;
}

bool FrameTargetTransfers::EnsureReadbackRequested() noexcept
{
    if (m_readbackRequested)
        return true;
    if (!mu::GetRenderer().RequestFramePixelsRgba8(false))
        return false;
    m_readbackRequested = true;
    return true;
}

bool FrameTargetTransfers::ExecuteCopyTarget(const RenderTapeCopyTargetRequest& request) noexcept
{
    std::uint32_t frameWidth = 0, frameHeight = 0;
    if (!mu::GetRenderer().GetFrameSize(frameWidth, frameHeight))
        return false;

    RenderTapeRect rect{};
    if (!NormalizeRect(request.rect, frameWidth, frameHeight, rect))
        return false;

    if (rect.x == 0 && rect.y == 0 &&
        rect.width == static_cast<int>(frameWidth) &&
        rect.height == static_cast<int>(frameHeight))
    {
        const std::uint32_t captured = mu::GetRenderer().CaptureFrameTexture(0);
        return captured != 0 &&
               GetLogicalRenderAssetTable().RegisterCapturedTexture(
                   request.destination, captured, frameWidth, frameHeight,
                   RenderAssetRetention::Shared, {});
    }

    std::scoped_lock lock(m_mutex);
    if (!EnsureReadbackRequested())
        return false;
    auto normalized = request;
    normalized.rect = rect;
    m_pendingCopies.push_back(normalized);
    return true;
}

bool FrameTargetTransfers::QueueDownload(const RenderTapeDownloadTargetRequest& request) noexcept
{
    if (request.requestId == 0)
        return false;

    std::uint32_t frameWidth = 0, frameHeight = 0;
    if (!mu::GetRenderer().GetFrameSize(frameWidth, frameHeight))
        return false;

    RenderTapeRect rect{};
    if (!NormalizeRect(request.rect, frameWidth, frameHeight, rect))
        return false;

    std::scoped_lock lock(m_mutex);
    if (m_completedDownloads.contains(request.requestId) ||
        std::any_of(m_pendingDownloads.begin(), m_pendingDownloads.end(),
                    [&](const auto& p) { return p.requestId == request.requestId; }))
        return false;

    if (!EnsureReadbackRequested())
        return false;

    auto normalized = request;
    normalized.rect = rect;
    m_pendingDownloads.push_back(normalized);
    return true;
}

void FrameTargetTransfers::CompleteFrame() noexcept
{
    std::scoped_lock lock(m_mutex);
    if (!m_readbackRequested)
        return;

    mu::FramePixelsRgba8 frame;
    if (!mu::GetRenderer().ConsumeFramePixelsRgba8(frame))
    {
        // The SDL GPU readback completes synchronously in EndFrame. If no
        // result exists here, the request failed and must not poison the next
        // frame by remaining permanently marked as pending.
        m_pendingCopies.clear();
        m_pendingDownloads.clear();
        m_readbackRequested = false;
        return;
    }

    for (const auto& request : m_pendingCopies)
    {
        std::vector<std::uint8_t> cropped;
        if (!Crop(frame, request.rect, false, cropped))
            continue;
        const auto bytes = std::span<const std::byte>(
            reinterpret_cast<const std::byte*>(cropped.data()), cropped.size());
        (void)GetLogicalRenderAssetTable().DefineTexture2D(
            request.destination,
            static_cast<std::uint32_t>(request.rect.width),
            static_cast<std::uint32_t>(request.rect.height),
            bytes, LegacyPixelFormat::Rgba8, RenderAssetRetention::Shared, {});
    }

    for (const auto& request : m_pendingDownloads)
    {
        CompletedTargetDownload completed{};
        completed.session = request.session;
        completed.generation = request.generation;
        completed.targetId = request.targetId;
        completed.requestId = request.requestId;
        completed.userToken = request.userToken;
        completed.width = static_cast<std::uint32_t>(request.rect.width);
        completed.height = static_cast<std::uint32_t>(request.rect.height);
        if (!Crop(frame, request.rect, request.reverseRows, completed.rgba))
            continue;
        m_completedDownloads[completed.requestId] = std::move(completed);
    }

    m_pendingCopies.clear();
    m_pendingDownloads.clear();
    m_readbackRequested = false;
}

bool FrameTargetTransfers::ConsumeDownload(std::uint64_t requestId,
                                           CompletedTargetDownload& download) noexcept
{
    std::scoped_lock lock(m_mutex);
    const auto it = m_completedDownloads.find(requestId);
    if (it == m_completedDownloads.end())
        return false;
    download = std::move(it->second);
    m_completedDownloads.erase(it);
    return true;
}

void FrameTargetTransfers::Reset() noexcept
{
    std::scoped_lock lock(m_mutex);
    m_readbackRequested = false;
    m_pendingCopies.clear();
    m_pendingDownloads.clear();
    m_completedDownloads.clear();
}

FrameTargetTransfers& GetFrameTargetTransfers()
{
    static FrameTargetTransfers transfers;
    return transfers;
}

} // namespace mu::pipeline

#pragma once

#include "Render/Renderer/FramePixelReadback.h"
#include "client/render/RenderTapeTypes.h"

#include <cstdint>
#include <mutex>
#include <unordered_map>
#include <vector>

namespace mu::pipeline
{

struct CompletedTargetDownload
{
    SessionId session{};
    SessionGeneration generation{};
    std::uint64_t targetId = 0;
    std::uint64_t requestId = 0;
    std::uint64_t userToken = 0;
    std::uint32_t width = 0;
    std::uint32_t height = 0;
    std::vector<std::uint8_t> rgba;
};

class FrameTargetTransfers
{
public:
    [[nodiscard]] bool ExecuteCopyTarget(const RenderTapeCopyTargetRequest& request) noexcept;
    [[nodiscard]] bool QueueDownload(const RenderTapeDownloadTargetRequest& request) noexcept;
    void CompleteFrame() noexcept;
    [[nodiscard]] bool ConsumeDownload(std::uint64_t requestId,
                                       CompletedTargetDownload& download) noexcept;
    void Reset() noexcept;

private:
    [[nodiscard]] bool EnsureReadbackRequested() noexcept;
    [[nodiscard]] static bool NormalizeRect(const RenderTapeRect& requested,
                                            std::uint32_t frameWidth,
                                            std::uint32_t frameHeight,
                                            RenderTapeRect& normalized) noexcept;
    [[nodiscard]] static bool Crop(const mu::FramePixelsRgba8& frame,
                                   const RenderTapeRect& rect,
                                   bool reverseRows,
                                   std::vector<std::uint8_t>& rgba) noexcept;

    std::mutex m_mutex;
    bool m_readbackRequested = false;
    std::vector<RenderTapeCopyTargetRequest> m_pendingCopies;
    std::vector<RenderTapeDownloadTargetRequest> m_pendingDownloads;
    std::unordered_map<std::uint64_t, CompletedTargetDownload> m_completedDownloads;
};

FrameTargetTransfers& GetFrameTargetTransfers();

} // namespace mu::pipeline

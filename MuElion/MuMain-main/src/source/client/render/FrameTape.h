#pragma once

#include <cstddef>
#include <cstdint>
#include <deque>
#include <mutex>
#include <optional>
#include <vector>

namespace mu::pipeline
{

enum class FrameRejectionReason : std::uint32_t
{
    None = 0,
    MissingAsset = 1,
    AssetRevisionMismatch = 2,
    InvalidSession = 3,
    QueueOverflow = 4,
};

struct FrameTapeSample
{
    std::uint64_t frame = 0;
    std::uint32_t session = 0;
    std::uint32_t renderJobs = 0;
    std::uint64_t beginNs = 0;
    std::uint64_t presentedNs = 0;
    std::uint64_t sealedNs = 0;
};

class FrameTape
{
public:
    static constexpr std::size_t DefaultCapacity = 240;

    explicit FrameTape(std::size_t capacity = DefaultCapacity);

    void BeginFrame(std::uint64_t frame, std::uint32_t session);
    void RecordRenderJob();
    void MarkPresented();
    void SealFrame();

    void RejectAsset(std::uint64_t assetId, std::uint64_t assetRevision,
                     FrameRejectionReason reason, std::uint32_t session);

    [[nodiscard]] std::vector<FrameTapeSample> Snapshot(std::size_t maxSamples = DefaultCapacity) const;
    [[nodiscard]] std::uint64_t RejectionCount() const;
    [[nodiscard]] std::size_t Capacity() const noexcept { return m_capacity; }

private:
    static std::uint64_t NowNs();

    const std::size_t m_capacity;
    mutable std::mutex m_mutex;
    std::deque<FrameTapeSample> m_samples;
    std::optional<FrameTapeSample> m_current;
    std::uint64_t m_rejectionCount = 0;
};

FrameTape& GetFrameTape();

} // namespace mu::pipeline

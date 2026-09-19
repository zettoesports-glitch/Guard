#include "stdafx.h"
#include "client/render/FrameTape.h"

#include <algorithm>
#include <chrono>

namespace mu::pipeline
{

const char* FrameRejectionReasonName(FrameRejectionReason reason) noexcept
{
    switch (reason)
    {
    case FrameRejectionReason::InvalidTapeIdentity:
        return "invalid-tape-identity";
    case FrameRejectionReason::MissingLogicalAsset:
        return "missing-logical-asset";
    case FrameRejectionReason::InvalidTapeContents:
        return "invalid-tape-contents";
    case FrameRejectionReason::AssetTableMismatch:
        return "asset-table-mismatch";
    default:
        return "unknown";
    }
}

FrameTape::FrameTape(std::size_t capacity)
    : m_capacity(std::max<std::size_t>(1, capacity))
{
}

std::uint64_t FrameTape::NowNs()
{
    return static_cast<std::uint64_t>(
        std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::steady_clock::now().time_since_epoch())
            .count());
}

void FrameTape::BeginFrame(std::uint64_t frame, std::uint32_t session)
{
    std::scoped_lock lock(m_mutex);

    // Never silently discard an unfinished frame. If a caller begins another
    // frame, close the previous sample so diagnostics remain monotonic.
    if (m_current)
    {
        m_current->sealedNs = NowNs();
        m_samples.push_back(*m_current);
        if (m_samples.size() > m_capacity)
            m_samples.pop_front();
    }

    FrameTapeSample sample{};
    sample.frame = frame;
    sample.session = session;
    sample.beginNs = NowNs();
    m_current = sample;
}

void FrameTape::RecordRenderJob()
{
    std::scoped_lock lock(m_mutex);
    if (m_current)
        ++m_current->renderJobs;
}

void FrameTape::MarkPresented()
{
    std::scoped_lock lock(m_mutex);
    if (m_current)
        m_current->presentedNs = NowNs();
}

void FrameTape::SealFrame()
{
    std::scoped_lock lock(m_mutex);
    if (!m_current)
        return;

    m_current->sealedNs = NowNs();
    m_samples.push_back(*m_current);
    m_current.reset();

    if (m_samples.size() > m_capacity)
        m_samples.pop_front();
}

void FrameTape::RejectAsset(std::uint64_t assetId, std::uint64_t assetRevision,
                            FrameRejectionReason reason, std::uint32_t session)
{
    std::scoped_lock lock(m_mutex);

    FrameTapeRejection rejection{};
    rejection.frame = m_current ? m_current->frame : 0;
    rejection.session = session;
    rejection.assetId = assetId;
    rejection.assetRevision = assetRevision;
    rejection.reason = reason;
    m_lastRejection = rejection;
    ++m_rejectionCount;
}

std::vector<FrameTapeSample> FrameTape::Snapshot(std::size_t maxSamples) const
{
    std::scoped_lock lock(m_mutex);

    const std::size_t count = std::min(maxSamples, m_samples.size());
    std::vector<FrameTapeSample> result;
    result.reserve(count);

    const auto first = m_samples.end() - static_cast<std::ptrdiff_t>(count);
    result.insert(result.end(), first, m_samples.end());
    return result;
}

std::optional<FrameTapeRejection> FrameTape::LastRejection() const
{
    std::scoped_lock lock(m_mutex);
    return m_lastRejection;
}

std::uint64_t FrameTape::RejectionCount() const
{
    std::scoped_lock lock(m_mutex);
    return m_rejectionCount;
}

FrameTape& GetFrameTape()
{
    static FrameTape tape;
    return tape;
}

} // namespace mu::pipeline

#include "stdafx.h"
#include "client/session/SessionRender.h"

#include "client/render/FrameTape.h"
#include "client/render/LegacyRenderFacade.h"
#include "client/render/FrameTargetTransfers.h"
#include "Render/Textures/ZzzOpenglUtil.h"
#ifdef MU_ENABLE_MODERN_UI
#include "UI/Modern/RmlUiRuntime.h"
#endif

#include <algorithm>
#include <utility>

namespace mu::session
{

bool SessionRenderUnit::BeginRenderTapePass(mu::pipeline::RenderTapePass pass) noexcept
{
    mu::pipeline::SessionFogPassConstants fog{};
    return mu::pipeline::GetLegacyRenderFacade().BeginPass(pass, fog);
}

#ifdef MU_ENABLE_MODERN_UI
bool SessionRenderUnit::SubmitModernUiPreparation(
    UI::Modern::RmlUiRuntime& runtime) noexcept
{
    if (!runtime.IsInitialized())
        return false;

    auto& facade = mu::pipeline::GetLegacyRenderFacade();
    const bool ownsPass = !facade.IsRecording();
    if (ownsPass &&
        !BeginRenderTapePass(mu::pipeline::RenderTapePass::Ui))
        return false;

    const auto snapshot = runtime.PrepareRenderSnapshot();
    const bool success =
        snapshot && snapshot->updateSucceeded && snapshot->renderSucceeded;

    if (ownsPass && !facade.EndPass())
        return false;

    return success;
}
#endif

void SessionRenderUnit::RenderPointRotate(
    int texture,
    float ix, float iy,
    float inputWidth, float inputHeight,
    float x, float y,
    float width, float height,
    float rotate, float rotateLocal,
    float uWidth, float vHeight,
    int number)
{
    ::RenderPointRotate(texture, ix, iy, inputWidth, inputHeight,
                        x, y, width, height, rotate, rotateLocal,
                        uWidth, vHeight, number);
}

SessionRender::~SessionRender()
{
    Shutdown();
}

void SessionRender::Configure(bool pipelineEnabled, std::size_t workerCount)
{
    Shutdown();

    m_pipelineEnabled = pipelineEnabled;
    if (!m_pipelineEnabled)
        return;

    const std::size_t count = std::clamp<std::size_t>(workerCount, 2, 64);
    {
        std::scoped_lock lock(m_mutex);
        m_stopping = false;
    }

    m_workers.reserve(count);
    for (std::size_t i = 0; i < count; ++i)
        m_workers.emplace_back(&SessionRender::WorkerLoop, this);
}

void SessionRender::Shutdown()
{
    {
        std::scoped_lock lock(m_mutex);
        m_stopping = true;
    }
    m_workAvailable.notify_all();

    for (auto& worker : m_workers)
    {
        if (worker.joinable())
            worker.join();
    }

    m_workers.clear();

    {
        std::scoped_lock lock(m_mutex);
        m_jobs.clear();
        m_pendingJobs = 0;
        m_stopping = false;
    }

    m_pipelineEnabled = false;
    mu::pipeline::GetFrameTargetTransfers().Reset();
}

void SessionRender::BeginFrame(std::uint64_t sessionId,
                               std::uint64_t generation,
                               std::uint64_t targetId)
{
    const std::uint64_t frame =
        m_frameCounter.fetch_add(1, std::memory_order_relaxed) + 1;

    mu::pipeline::GetLegacyRenderFacade().SetFrameIdentity(
        mu::pipeline::SessionId(sessionId),
        mu::pipeline::SessionGeneration(generation),
        targetId);

    mu::pipeline::GetFrameTape().BeginFrame(
        frame, static_cast<std::uint32_t>(sessionId));
}

void SessionRender::Submit(Job job)
{
    if (!job)
        return;

    mu::pipeline::GetFrameTape().RecordRenderJob();

    if (!m_pipelineEnabled)
    {
        job();
        return;
    }

    {
        std::scoped_lock lock(m_mutex);
        m_jobs.push_back(std::move(job));
        ++m_pendingJobs;
    }
    m_workAvailable.notify_one();
}

void SessionRender::WaitIdle()
{
    if (!m_pipelineEnabled)
        return;

    std::unique_lock lock(m_mutex);
    m_idle.wait(lock, [this] { return m_pendingJobs == 0 && m_jobs.empty(); });
}

void SessionRender::ReplayFrame()
{
    WaitIdle();

    if (!m_pipelineEnabled)
        return;

    auto tape = mu::pipeline::GetLegacyRenderFacade().Finalize();
    if (tape && !tape->Replay())
    {
        mu::pipeline::GetFrameTape().RejectAsset(
            0, 0, mu::pipeline::FrameRejectionReason::InvalidTapeContents, 0);
    }
}

void SessionRender::EndFrame()
{
    mu::pipeline::GetFrameTargetTransfers().CompleteFrame();
    mu::pipeline::GetFrameTape().MarkPresented();
    mu::pipeline::GetFrameTape().SealFrame();
}

bool SessionRender::ConsumeDownloadedTarget(
    std::uint64_t requestId,
    mu::pipeline::CompletedTargetDownload& download) noexcept
{
    return mu::pipeline::GetFrameTargetTransfers().ConsumeDownload(
        requestId, download);
}

void SessionRender::WorkerLoop()
{
    for (;;)
    {
        Job job;
        {
            std::unique_lock lock(m_mutex);
            m_workAvailable.wait(lock, [this] { return m_stopping || !m_jobs.empty(); });

            if (m_stopping && m_jobs.empty())
                return;

            job = std::move(m_jobs.front());
            m_jobs.pop_front();
        }

        try
        {
            job();
        }
        catch (...)
        {
            // Render-preparation work must not terminate a worker thread.
            // The caller-facing pipeline remains alive and drains normally.
        }

        {
            std::scoped_lock lock(m_mutex);
            if (m_pendingJobs > 0)
                --m_pendingJobs;
            if (m_pendingJobs == 0 && m_jobs.empty())
                m_idle.notify_all();
        }
    }
}

SessionRender& GetSessionRender()
{
    static SessionRender sessionRender;
    return sessionRender;
}

} // namespace mu::session

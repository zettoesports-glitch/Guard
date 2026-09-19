#pragma once

#include <atomic>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <functional>
#include <mutex>
#include <thread>
#include <vector>

namespace mu::pipeline
{
enum class RenderTapePass : std::uint8_t;
}

namespace mu::session
{

class SessionRenderUnit
{
public:
    [[nodiscard]] bool BeginRenderTapePass(mu::pipeline::RenderTapePass pass) noexcept;
};

class SessionRender
{
public:
    using Job = std::function<void()>;

    SessionRender() = default;
    ~SessionRender();

    SessionRender(const SessionRender&) = delete;
    SessionRender& operator=(const SessionRender&) = delete;

    // pipelineEnabled=false preserves the legacy immediate path.
    void Configure(bool pipelineEnabled, std::size_t workerCount);
    void Shutdown();

    void BeginFrame(std::uint32_t sessionId = 0);
    void Submit(Job job);
    void WaitIdle();
    void ReplayFrame();
    void EndFrame();

    [[nodiscard]] bool IsPipelineEnabled() const noexcept { return m_pipelineEnabled; }
    [[nodiscard]] std::size_t WorkerCount() const noexcept { return m_workers.size(); }
    [[nodiscard]] std::uint64_t CurrentFrame() const noexcept { return m_frameCounter.load(); }

private:
    void WorkerLoop();

    bool m_pipelineEnabled = false;
    bool m_stopping = false;

    std::atomic<std::uint64_t> m_frameCounter{0};

    std::mutex m_mutex;
    std::condition_variable m_workAvailable;
    std::condition_variable m_idle;
    std::deque<Job> m_jobs;
    std::vector<std::thread> m_workers;
    std::size_t m_pendingJobs = 0;
};

SessionRender& GetSessionRender();

} // namespace mu::session

#pragma once

#include <cstdint>
#include <functional>
#include <memory>

namespace Rml
{
class Context;
}

namespace UI::Modern
{

struct RmlUiRenderSnapshot
{
    std::uint64_t sequence = 0;
    int width = 0;
    int height = 0;
    float densityIndependentPixelRatio = 1.0f;
    bool updateSucceeded = false;
    bool renderSucceeded = false;
};

class RmlUiRuntime
{
public:
    RmlUiRuntime();
    ~RmlUiRuntime();

    RmlUiRuntime(const RmlUiRuntime&) = delete;
    RmlUiRuntime& operator=(const RmlUiRuntime&) = delete;

    [[nodiscard]] bool Initialize(int width, int height, int scalePercent);
    void Shutdown();

    [[nodiscard]] bool IsInitialized() const noexcept;
    [[nodiscard]] bool Resize(int width, int height);
    [[nodiscard]] bool SetScalePercent(int scalePercent);

    // The private Debug executable exposes Impl::Execute(std::function<void()>)
    // returning bool. The public wrapper retains the same execution contract.
    [[nodiscard]] bool Execute(std::function<void()> operation);

    [[nodiscard]] bool Update();
    [[nodiscard]] bool Render();
    [[nodiscard]] std::shared_ptr<const RmlUiRenderSnapshot> PrepareRenderSnapshot();

    [[nodiscard]] Rml::Context* GetContext() noexcept;

private:
    class Impl;
    std::unique_ptr<Impl> m_impl;
};

RmlUiRuntime& GetRmlUiRuntime();

} // namespace UI::Modern

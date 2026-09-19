#include "stdafx.h"
#include "UI/Modern/RmlUiRuntime.h"

#include "UI/Modern/TapeRenderInterface.h"
#include "UI/Modern/RmlHudMapViewport.h"

#include <RmlUi/Core/Context.h>
#include <RmlUi/Core/Core.h>
#include <RmlUi/Core/ElementDocument.h>
#include <RmlUi/Core/ElementInstancer.h>
#include <RmlUi/Core/Factory.h>
#include <RmlUi/Core/SystemInterface.h>

#include <algorithm>
#include <functional>
#include <mutex>
#include <string>
#include <unordered_map>
#include <utility>

namespace UI::Modern
{

namespace
{
// RTTI for this private class name is present in Main-x64-Debug.exe.
class MuSystemInterface final : public Rml::SystemInterface
{
};
} // namespace

class RmlUiRuntime::Impl
{
public:
    explicit Impl(TapeRenderInterface& renderInterface) noexcept
        : renderInterface_(renderInterface)
    {
    }

    ~Impl()
    {
        Shutdown();
    }

    [[nodiscard]] bool Initialize(int width, int height, int scalePercent)
    {
        std::scoped_lock lock(mutex_);
        if (width <= 0 || height <= 0)
            return false;

        scalePercent = std::clamp(scalePercent, 100, 200);

        if (initialized_)
        {
            context_->SetDimensions({width, height});
            context_->SetDensityIndependentPixelRatio(
                static_cast<float>(scalePercent) / 100.0f);
            scalePercent_ = scalePercent;
            return true;
        }

        Rml::SetSystemInterface(&systemInterface_);
        Rml::SetRenderInterface(&renderInterface_);

        if (!Rml::Initialise())
        {
            Rml::SetRenderInterface(nullptr);
            Rml::SetSystemInterface(nullptr);
            return false;
        }

        mapViewportInstancer_ =
            std::make_unique<Rml::ElementInstancerGeneric<RmlHudMapViewport>>();
        Rml::Factory::RegisterElementInstancer(
            "map-view", mapViewportInstancer_.get());
        Rml::Factory::RegisterElementInstancer(
            "map-viewport", mapViewportInstancer_.get());

        context_ = Rml::CreateContext(
            contextName_, {width, height}, &renderInterface_);
        if (!context_)
        {
            mapViewportInstancer_.reset();
            Rml::Shutdown();
            Rml::SetRenderInterface(nullptr);
            Rml::SetSystemInterface(nullptr);
            return false;
        }

        context_->SetDensityIndependentPixelRatio(
            static_cast<float>(scalePercent) / 100.0f);
        scalePercent_ = scalePercent;

        // The Debug executable names Data/UI/PC/HUD/main_frame.rml as the HUD
        // root. Keep bootstrap loading optional so the legacy UI still works
        // while additional RmlUi documents are reconstructed incrementally.
        bootstrapDocument_ =
            context_->LoadDocument("Data/UI/PC/HUD/main_frame.rml");
        if (bootstrapDocument_)
        {
            documents_.emplace(
                "Data/UI/PC/HUD/main_frame.rml", bootstrapDocument_);
            bootstrapDocument_->Show();
        }

        initialized_ = true;
        return true;
    }

    void Shutdown() noexcept
    {
        std::scoped_lock lock(mutex_);
        if (!initialized_)
            return;

        if (context_)
        {
            documents_.clear();
            Rml::RemoveContext(contextName_);
            context_ = nullptr;
            bootstrapDocument_ = nullptr;
        }

        mapViewportInstancer_.reset();
        Rml::Shutdown();
        Rml::SetRenderInterface(nullptr);
        Rml::SetSystemInterface(nullptr);

        lastSnapshot_.reset();
        initialized_ = false;
        sequence_ = 0;
    }

    [[nodiscard]] bool IsInitialized() const noexcept
    {
        std::scoped_lock lock(mutex_);
        return initialized_ && context_ != nullptr;
    }

    [[nodiscard]] bool Resize(int width, int height)
    {
        if (width <= 0 || height <= 0)
            return false;

        return Execute([this, width, height] {
            context_->SetDimensions({width, height});
        });
    }

    [[nodiscard]] bool SetScalePercent(int scalePercent)
    {
        scalePercent = std::clamp(scalePercent, 100, 200);
        return Execute([this, scalePercent] {
            scalePercent_ = scalePercent;
            context_->SetDensityIndependentPixelRatio(
                static_cast<float>(scalePercent_) / 100.0f);
        });
    }

    [[nodiscard]] bool Execute(std::function<void()> operation)
    {
        if (!operation)
            return false;

        std::scoped_lock lock(mutex_);
        if (!initialized_ || !context_)
            return false;

        try
        {
            operation();
            return true;
        }
        catch (...)
        {
            return false;
        }
    }

    [[nodiscard]] bool Update()
    {
        bool result = false;
        const bool executed = Execute([this, &result] {
            result = context_->Update();
        });
        return executed && result;
    }

    [[nodiscard]] bool Render()
    {
        bool result = false;
        const bool executed = Execute([this, &result] {
            result = context_->Render();
        });
        return executed && result;
    }

    [[nodiscard]] std::shared_ptr<const RmlUiRenderSnapshot> PrepareRenderSnapshot()
    {
        std::shared_ptr<RmlUiRenderSnapshot> snapshot;
        const bool executed = Execute([this, &snapshot] {
            snapshot = std::make_shared<RmlUiRenderSnapshot>();
            snapshot->sequence = ++sequence_;

            const Rml::Vector2i dimensions = context_->GetDimensions();
            snapshot->width = dimensions.x;
            snapshot->height = dimensions.y;
            snapshot->densityIndependentPixelRatio =
                context_->GetDensityIndependentPixelRatio();
            snapshot->updateSucceeded = context_->Update();
            snapshot->renderSucceeded =
                snapshot->updateSucceeded && context_->Render();

            lastSnapshot_ = snapshot;
        });

        return executed ? snapshot : nullptr;
    }

    [[nodiscard]] Rml::ElementDocument* LoadDocument(
        const char* path, bool show)
    {
        if (path == nullptr || *path == '\0')
            return nullptr;

        Rml::ElementDocument* document = nullptr;
        const bool executed = Execute([this, path, show, &document] {
            const auto existing = documents_.find(path);
            if (existing != documents_.end())
            {
                document = existing->second;
            }
            else
            {
                document = context_->LoadDocument(path);
                if (document)
                    documents_.emplace(path, document);
            }

            if (document && show)
                document->Show();
        });

        return executed ? document : nullptr;
    }

    [[nodiscard]] bool ShowDocument(const char* path)
    {
        return LoadDocument(path, true) != nullptr;
    }

    [[nodiscard]] bool HideDocument(const char* path)
    {
        if (path == nullptr || *path == '\0')
            return false;

        bool hidden = false;
        const bool executed = Execute([this, path, &hidden] {
            const auto existing = documents_.find(path);
            if (existing == documents_.end() || existing->second == nullptr)
                return;

            existing->second->Hide();
            hidden = true;
        });

        return executed && hidden;
    }

    [[nodiscard]] Rml::ElementDocument* GetDocument(const char* path) noexcept
    {
        if (path == nullptr || *path == '\0')
            return nullptr;

        std::scoped_lock lock(mutex_);
        if (!initialized_ || !context_)
            return nullptr;

        const auto existing = documents_.find(path);
        return existing != documents_.end() ? existing->second : nullptr;
    }

    [[nodiscard]] Rml::Context* GetContext() noexcept
    {
        std::scoped_lock lock(mutex_);
        return context_;
    }

private:
    mutable std::recursive_mutex mutex_;
    MuSystemInterface systemInterface_;
    TapeRenderInterface& renderInterface_;
    Rml::Context* context_ = nullptr;
    Rml::ElementDocument* bootstrapDocument_ = nullptr;
    Rml::String contextName_ = "mu-main-modern";
    int scalePercent_ = 100;
    bool initialized_ = false;
    std::uint64_t sequence_ = 0;
    std::shared_ptr<RmlUiRenderSnapshot> lastSnapshot_;
    std::unordered_map<std::string, Rml::ElementDocument*> documents_;
    std::unique_ptr<Rml::ElementInstancerGeneric<RmlHudMapViewport>>
        mapViewportInstancer_;
};

RmlUiRuntime::RmlUiRuntime()
    : m_impl(std::make_unique<Impl>(GetTapeRenderInterface()))
{
}

RmlUiRuntime::~RmlUiRuntime() = default;

bool RmlUiRuntime::Initialize(int width, int height, int scalePercent)
{
    return m_impl && m_impl->Initialize(width, height, scalePercent);
}

void RmlUiRuntime::Shutdown()
{
    if (m_impl)
        m_impl->Shutdown();
}

bool RmlUiRuntime::IsInitialized() const noexcept
{
    return m_impl && m_impl->IsInitialized();
}

bool RmlUiRuntime::Resize(int width, int height)
{
    return m_impl && m_impl->Resize(width, height);
}

bool RmlUiRuntime::SetScalePercent(int scalePercent)
{
    return m_impl && m_impl->SetScalePercent(scalePercent);
}

bool RmlUiRuntime::Execute(std::function<void()> operation)
{
    return m_impl && m_impl->Execute(std::move(operation));
}

bool RmlUiRuntime::Update()
{
    return m_impl && m_impl->Update();
}

bool RmlUiRuntime::Render()
{
    return m_impl && m_impl->Render();
}

std::shared_ptr<const RmlUiRenderSnapshot> RmlUiRuntime::PrepareRenderSnapshot()
{
    return m_impl ? m_impl->PrepareRenderSnapshot() : nullptr;
}

Rml::Context* RmlUiRuntime::GetContext() noexcept
{
    return m_impl ? m_impl->GetContext() : nullptr;
}

Rml::ElementDocument* RmlUiRuntime::LoadDocument(
    const char* path, bool show)
{
    return m_impl ? m_impl->LoadDocument(path, show) : nullptr;
}

bool RmlUiRuntime::ShowDocument(const char* path)
{
    return m_impl && m_impl->ShowDocument(path);
}

bool RmlUiRuntime::HideDocument(const char* path)
{
    return m_impl && m_impl->HideDocument(path);
}

Rml::ElementDocument* RmlUiRuntime::GetDocument(const char* path) noexcept
{
    return m_impl ? m_impl->GetDocument(path) : nullptr;
}

RmlUiRuntime& GetRmlUiRuntime()
{
    static RmlUiRuntime runtime;
    return runtime;
}

} // namespace UI::Modern

#include "stdafx.h"
#include "UI/Modern/RmlDocumentHost.h"

#include "UI/Modern/RmlUiRuntime.h"

#include <RmlUi/Core/ElementDocument.h>

#include <functional>
#include <utility>

namespace UI::Modern
{

class RmlDocumentHost::Impl
{
public:
    Impl(RmlUiRuntime& runtime, std::string path)
        : runtime_(runtime), path_(std::move(path))
    {
    }

    ~Impl()
    {
        Release();
    }

    [[nodiscard]] bool Load(bool show)
    {
        if (path_.empty())
            return false;

        if (document_)
        {
            if (show)
                return Show();
            return true;
        }

        document_ = runtime_.LoadDocument(path_.c_str(), show);
        return document_ != nullptr;
    }

    [[nodiscard]] bool Show()
    {
        if (!document_ && !Load(false))
            return false;

        return runtime_.ShowDocument(path_.c_str());
    }

    [[nodiscard]] bool Hide()
    {
        if (!document_)
            return false;

        return runtime_.HideDocument(path_.c_str());
    }

    void Release()
    {
        if (!document_ || path_.empty())
            return;

        // Main-x64-Debug.exe preserves the concrete lambda type inside
        // RmlDocumentHost::Impl::Release. Execute the release through the
        // runtime just like that recovered architecture, rather than closing
        // the document from an arbitrary caller thread.
        const std::string path = path_;
        (void)runtime_.Execute([this, path] {
            (void)runtime_.ReleaseDocument(path.c_str());
            document_ = nullptr;
        });

        // If the runtime is already shut down Execute() returns false. Its
        // context teardown owns any remaining RmlUi documents, so never retain
        // a pointer past this host release either.
        document_ = nullptr;
    }

    [[nodiscard]] bool IsLoaded() const noexcept
    {
        return document_ != nullptr;
    }

    [[nodiscard]] Rml::ElementDocument* GetDocument() const noexcept
    {
        return document_;
    }

    [[nodiscard]] const std::string& GetPath() const noexcept
    {
        return path_;
    }

private:
    RmlUiRuntime& runtime_;
    std::string path_;
    Rml::ElementDocument* document_ = nullptr;
};

RmlDocumentHost::RmlDocumentHost(
    RmlUiRuntime& runtime, std::string path)
    : m_impl(std::make_unique<Impl>(runtime, std::move(path)))
{
}

RmlDocumentHost::RmlDocumentHost(std::string path)
    : RmlDocumentHost(GetRmlUiRuntime(), std::move(path))
{
}

RmlDocumentHost::~RmlDocumentHost() = default;
RmlDocumentHost::RmlDocumentHost(RmlDocumentHost&&) noexcept = default;
RmlDocumentHost& RmlDocumentHost::operator=(RmlDocumentHost&&) noexcept = default;

bool RmlDocumentHost::Load(bool show)
{
    return m_impl && m_impl->Load(show);
}

bool RmlDocumentHost::Show()
{
    return m_impl && m_impl->Show();
}

bool RmlDocumentHost::Hide()
{
    return m_impl && m_impl->Hide();
}

void RmlDocumentHost::Release()
{
    if (m_impl)
        m_impl->Release();
}

bool RmlDocumentHost::IsLoaded() const noexcept
{
    return m_impl && m_impl->IsLoaded();
}

Rml::ElementDocument* RmlDocumentHost::GetDocument() const noexcept
{
    return m_impl ? m_impl->GetDocument() : nullptr;
}

const std::string& RmlDocumentHost::GetPath() const noexcept
{
    static const std::string empty;
    return m_impl ? m_impl->GetPath() : empty;
}

} // namespace UI::Modern

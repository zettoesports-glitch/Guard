#pragma once

#include <memory>
#include <string>

namespace Rml
{
class ElementDocument;
}

namespace UI::Modern
{

class RmlUiRuntime;

// Reconstructed architectural wrapper for the RTTI-visible RmlDocumentHost.
// The Debug executable confirms the nested Impl type and Impl::Release()
// creating a std::function<void()> lambda. Private field names/layout are not
// preserved, so this class intentionally exposes only the observable document
// lifecycle contract instead of claiming byte-identical source recovery.
class RmlDocumentHost
{
public:
    RmlDocumentHost(RmlUiRuntime& runtime, std::string path);
    explicit RmlDocumentHost(std::string path);
    ~RmlDocumentHost();

    RmlDocumentHost(const RmlDocumentHost&) = delete;
    RmlDocumentHost& operator=(const RmlDocumentHost&) = delete;
    RmlDocumentHost(RmlDocumentHost&&) noexcept;
    RmlDocumentHost& operator=(RmlDocumentHost&&) noexcept;

    [[nodiscard]] bool Load(bool show = false);
    [[nodiscard]] bool Show();
    [[nodiscard]] bool Hide();
    void Release();

    [[nodiscard]] bool IsLoaded() const noexcept;
    [[nodiscard]] Rml::ElementDocument* GetDocument() const noexcept;
    [[nodiscard]] const std::string& GetPath() const noexcept;

private:
    class Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace UI::Modern

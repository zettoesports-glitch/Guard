#include "stdafx.h"
#include "UI/Modern/PC/ServerMessage/RmlServerMessageLayer.h"

#include "UI/Modern/RmlDocumentHost.h"
#include "UI/Modern/RmlUiDesign.h"

#include <RmlUi/Core/Core.h>
#include <RmlUi/Core/Element.h>
#include <RmlUi/Core/ElementDocument.h>
#include <RmlUi/Core/FileInterface.h>
#include <RmlUi/Core/StreamMemory.h>

#include <algorithm>
#include <array>
#include <cstdio>
#include <sstream>

namespace UI::Modern::PC::ServerMessage
{

namespace
{
constexpr const char* kDocumentPath =
    "Data/UI/PC/ServerMessage/server_message.rml";

template <typename T>
bool ParseScalar(
    const RmlUiDesign::Values& values,
    const char* key,
    T& output)
{
    const auto it = values.find(key);
    if (it == values.end())
        return false;
    std::istringstream stream(it->second);
    T parsed{};
    if (!(stream >> parsed))
        return false;
    output = parsed;
    return true;
}

RmlUiDesign::Values LoadDesignValues()
{
    Rml::FileInterface* files = Rml::GetFileInterface();
    if (!files)
        return {};
    Rml::String contents;
    if (!files->LoadFile(kDocumentPath, contents) || contents.empty())
        return {};
    Rml::StreamMemory stream(
        reinterpret_cast<const Rml::byte*>(contents.data()),
        contents.size());
    return RmlUiDesign::Parse(&stream);
}

Rml::String PixelValue(float value)
{
    std::array<char, 64> buffer{};
    std::snprintf(
        buffer.data(), buffer.size(), "%.3fpx",
        static_cast<double>(value));
    return Rml::String(buffer.data());
}

void SetText(Rml::Element* element, const std::string& text)
{
    if (!element)
        return;
    element->SetInnerRML("");
    if (Rml::ElementDocument* document = element->GetOwnerDocument())
        element->AppendChild(document->CreateTextNode(text));
}
} // namespace

class RmlServerMessageLayer::Impl
{
public:
    struct Design
    {
        float height = 200.0f;
        float width = 400.0f;
        float left = 10.0f;
        float topOffset = 10.0f;
        float referenceTop = 31.0f;
        float referenceHeight = 600.0f;
    };

    Impl() : host_(kDocumentPath) {}
    ~Impl() { Release(); }

    bool Load(bool show)
    {
        if (host_.IsLoaded())
        {
            if (show) return Show();
            return true;
        }
        if (!host_.Load(show))
            return false;
        document_ = host_.GetDocument();
        if (!document_)
            return false;

        const auto values = LoadDesignValues();
        (void)ParseScalar(values, "ServerMessage-Height", design_.height);
        (void)ParseScalar(values, "ServerMessage-Width", design_.width);
        (void)ParseScalar(values, "ServerMessage-Left", design_.left);
        (void)ParseScalar(values, "ServerMessage-TopOffset", design_.topOffset);
        (void)ParseScalar(values, "ServerMessage-ReferenceTop", design_.referenceTop);
        (void)ParseScalar(values, "ServerMessage-ReferenceHeight", design_.referenceHeight);

        panel_ = document_->GetElementById("server-message-panel");
        text_ = document_->GetElementById("server-message-text");
        if (!panel_ || !text_)
        {
            Release();
            return false;
        }

        hostVisible_ = show;
        return ApplyState(state_);
    }

    bool Show()
    {
        if (!host_.IsLoaded() && !Load(false))
            return false;
        hostVisible_ = true;
        const bool result = host_.Show();
        if (result) (void)ApplyState(state_);
        return result;
    }

    bool Hide()
    {
        hostVisible_ = false;
        if (panel_) panel_->SetProperty("display", "none");
        return host_.Hide();
    }

    void Release()
    {
        panel_ = text_ = nullptr;
        document_ = nullptr;
        host_.Release();
    }

    bool IsLoaded() const noexcept
    {
        return host_.IsLoaded() && document_ != nullptr;
    }

    bool ApplyState(const State& state)
    {
        state_ = state;
        if (!IsLoaded())
            return false;

        const bool visible =
            hostVisible_ && state_.visible &&
            state_.viewportWidth > 0 && state_.viewportHeight > 0;
        panel_->SetProperty("display", visible ? "block" : "none");
        if (!visible)
            return true;

        const float scale =
            design_.referenceHeight > 0.0f
                ? static_cast<float>(state_.viewportHeight) /
                      design_.referenceHeight
                : 1.0f;
        const float top =
            design_.referenceTop * scale + design_.topOffset;

        panel_->SetProperty("left", PixelValue(design_.left));
        panel_->SetProperty("top", PixelValue(top));
        panel_->SetProperty("width", PixelValue(design_.width));
        panel_->SetProperty("height", PixelValue(design_.height));
        text_->SetProperty("width", PixelValue(design_.width));
        text_->SetProperty("height", PixelValue(design_.height));
        SetText(text_, state_.text);
        document_->UpdateDocument();
        return true;
    }

private:
    RmlDocumentHost host_;
    Rml::ElementDocument* document_ = nullptr;
    Rml::Element* panel_ = nullptr;
    Rml::Element* text_ = nullptr;
    bool hostVisible_ = false;
    Design design_;
    State state_;
};

RmlServerMessageLayer::RmlServerMessageLayer()
    : m_impl(std::make_unique<Impl>()) {}
RmlServerMessageLayer::~RmlServerMessageLayer() = default;
RmlServerMessageLayer::RmlServerMessageLayer(
    RmlServerMessageLayer&&) noexcept = default;
RmlServerMessageLayer& RmlServerMessageLayer::operator=(
    RmlServerMessageLayer&&) noexcept = default;

bool RmlServerMessageLayer::Load(bool show) { return m_impl && m_impl->Load(show); }
bool RmlServerMessageLayer::Show() { return m_impl && m_impl->Show(); }
bool RmlServerMessageLayer::Hide() { return m_impl && m_impl->Hide(); }
void RmlServerMessageLayer::Release() { if (m_impl) m_impl->Release(); }
bool RmlServerMessageLayer::IsLoaded() const noexcept { return m_impl && m_impl->IsLoaded(); }
bool RmlServerMessageLayer::ApplyState(const State& state) { return m_impl && m_impl->ApplyState(state); }

} // namespace UI::Modern::PC::ServerMessage

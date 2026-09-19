#include "stdafx.h"
#include "UI/Modern/PC/Help/RmlLongNoticeLayer.h"

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

namespace UI::Modern::PC::Help
{
namespace
{
constexpr const char* kDocumentPath =
    "Data/UI/PC/Help/long_notice.rml";

Rml::String PixelValue(float value)
{
    std::array<char, 64> buffer{};
    std::snprintf(
        buffer.data(), buffer.size(), "%.3fpx",
        static_cast<double>(value));
    return Rml::String(buffer.data());
}

bool ParsePair(
    const RmlUiDesign::Values& values,
    const char* key,
    std::array<float, 2>& output)
{
    const auto it = values.find(key);
    if (it == values.end())
        return false;
    std::istringstream stream(it->second);
    return static_cast<bool>(stream >> output[0] >> output[1]);
}

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
    return static_cast<bool>(stream >> output);
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

void SetText(Rml::Element* element, const std::string& text)
{
    if (!element)
        return;
    element->SetInnerRML("");
    if (Rml::ElementDocument* document = element->GetOwnerDocument())
        element->AppendChild(document->CreateTextNode(text));
}

Rml::String ColourValue(std::uint32_t argb)
{
    const unsigned red = (argb >> 16u) & 0xffu;
    const unsigned green = (argb >> 8u) & 0xffu;
    const unsigned blue = argb & 0xffu;
    std::array<char, 16> buffer{};
    std::snprintf(
        buffer.data(), buffer.size(),
        "#%02x%02x%02x", red, green, blue);
    return Rml::String(buffer.data());
}
} // namespace

class RmlLongNoticeLayer::Impl
{
public:
    Impl() : host_(kDocumentPath) {}
    ~Impl() { Release(); }

    [[nodiscard]] bool Load(bool show)
    {
        if (host_.IsLoaded())
            return show ? Show() : true;
        if (!host_.Load(show))
            return false;

        document_ = host_.GetDocument();
        if (!document_)
            return false;

        const auto values = LoadDesignValues();
        (void)ParsePair(values, "Menu-Size", menuSize_);
        (void)ParsePair(values, "Menu-Reference", referenceSize_);
        (void)ParseScalar(
            values, "Notice-HoldMilliseconds", holdMilliseconds_);

        menu_ = document_->GetElementById("menu");
        background_ = document_->GetElementById("notice-background");
        text_ = document_->GetElementById("notice-text");
        if (!menu_ || !background_ || !text_)
        {
            Release();
            return false;
        }

        hostVisible_ = show;
        return ApplyState(state_);
    }

    [[nodiscard]] bool Show()
    {
        if (!host_.IsLoaded() && !Load(false))
            return false;
        hostVisible_ = true;
        const bool ok = host_.Show();
        if (ok)
            (void)ApplyState(state_);
        return ok;
    }

    [[nodiscard]] bool Hide()
    {
        hostVisible_ = false;
        if (menu_)
            menu_->SetProperty("display", "none");
        return host_.Hide();
    }

    void Release()
    {
        menu_ = background_ = text_ = nullptr;
        document_ = nullptr;
        host_.Release();
    }

    [[nodiscard]] bool IsLoaded() const noexcept
    {
        return host_.IsLoaded() && document_ != nullptr;
    }

    [[nodiscard]] bool ApplyState(const State& state)
    {
        state_ = state;
        if (!IsLoaded())
            return false;

        const bool visible =
            hostVisible_ &&
            state_.visible &&
            !state_.text.empty() &&
            state_.viewportWidth > 0 &&
            state_.viewportHeight > 0 &&
            state_.alphaRate > 0.0f;

        menu_->SetProperty("display", visible ? "block" : "none");
        if (!visible)
            return true;

        const float scaleX =
            referenceSize_[0] > 0.0f
                ? static_cast<float>(state_.viewportWidth) / referenceSize_[0]
                : 1.0f;
        const float scaleY =
            referenceSize_[1] > 0.0f
                ? static_cast<float>(state_.viewportHeight) / referenceSize_[1]
                : 1.0f;

        menu_->SetProperty(
            "top", PixelValue(state_.referenceY * scaleY));
        menu_->SetProperty(
            "height", PixelValue(menuSize_[1] * scaleY));

        SetText(text_, state_.text);
        text_->SetProperty(
            "left", PixelValue(state_.referenceX * scaleX));
        text_->SetProperty(
            "color", ColourValue(state_.textColor));

        const float textOpacity =
            static_cast<float>(state_.textAlpha) / 255.0f;
        const float frameOpacity =
            std::clamp(state_.alphaRate / 205.0f, 0.0f, 1.0f);

        std::array<char, 32> opacity{};
        std::snprintf(
            opacity.data(), opacity.size(), "%.3f",
            static_cast<double>(textOpacity));
        text_->SetProperty("opacity", opacity.data());

        std::snprintf(
            opacity.data(), opacity.size(), "%.3f",
            static_cast<double>(frameOpacity));
        background_->SetProperty("opacity", opacity.data());

        menu_->SetClass("notice-priority", state_.notice);
        document_->UpdateDocument();
        return true;
    }

    [[nodiscard]] bool Update()
    {
        return false;
    }

private:
    RmlDocumentHost host_;
    Rml::ElementDocument* document_ = nullptr;
    Rml::Element* menu_ = nullptr;
    Rml::Element* background_ = nullptr;
    Rml::Element* text_ = nullptr;
    bool hostVisible_ = false;

    std::array<float, 2> menuSize_{1024.0f, 37.0f};
    std::array<float, 2> referenceSize_{640.0f, 480.0f};
    int holdMilliseconds_ = 2000;
    State state_;
};

RmlLongNoticeLayer::RmlLongNoticeLayer()
    : m_impl(std::make_unique<Impl>()) {}
RmlLongNoticeLayer::~RmlLongNoticeLayer() = default;
RmlLongNoticeLayer::RmlLongNoticeLayer(RmlLongNoticeLayer&&) noexcept = default;
RmlLongNoticeLayer& RmlLongNoticeLayer::operator=(RmlLongNoticeLayer&&) noexcept = default;
bool RmlLongNoticeLayer::Load(bool show) { return m_impl && m_impl->Load(show); }
bool RmlLongNoticeLayer::Show() { return m_impl && m_impl->Show(); }
bool RmlLongNoticeLayer::Hide() { return m_impl && m_impl->Hide(); }
void RmlLongNoticeLayer::Release() { if (m_impl) m_impl->Release(); }
bool RmlLongNoticeLayer::IsLoaded() const noexcept { return m_impl && m_impl->IsLoaded(); }
bool RmlLongNoticeLayer::ApplyState(const State& state) { return m_impl && m_impl->ApplyState(state); }
bool RmlLongNoticeLayer::Update() { return m_impl && m_impl->Update(); }

} // namespace UI::Modern::PC::Help

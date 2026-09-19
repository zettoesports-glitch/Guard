#include "stdafx.h"
#include "UI/Modern/PC/World/RmlMapNameLayer.h"

#include "UI/Modern/RmlDocumentHost.h"
#include "UI/Modern/RmlUiDesign.h"

#include <RmlUi/Core/Core.h>
#include <RmlUi/Core/Element.h>
#include <RmlUi/Core/ElementDocument.h>
#include <RmlUi/Core/FileInterface.h>
#include <RmlUi/Core/StreamMemory.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <cstdio>
#include <sstream>
#include <utility>

namespace UI::Modern::PC::World
{

namespace
{
constexpr const char* kDocumentPath = "Data/UI/PC/World/map_name.rml";

Rml::String PixelValue(float value)
{
    std::array<char, 64> buffer{};
    std::snprintf(
        buffer.data(), buffer.size(), "%.3fpx",
        static_cast<double>(value));
    return Rml::String(buffer.data());
}

Rml::String ScalarValue(float value)
{
    std::array<char, 32> buffer{};
    std::snprintf(
        buffer.data(), buffer.size(), "%.4f",
        static_cast<double>(value));
    return Rml::String(buffer.data());
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
    T parsed{};
    if (!(stream >> parsed))
        return false;
    output = parsed;
    return true;
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

void SetText(Rml::Element* element, const std::string& value)
{
    if (!element)
        return;
    element->SetInnerRML("");
    if (!value.empty())
        if (Rml::ElementDocument* document = element->GetOwnerDocument())
            element->AppendChild(document->CreateTextNode(value));
}
} // namespace

class RmlMapNameLayer::Impl
{
public:
    struct Design
    {
        std::array<float, 2> panelSize{256.811462f, 165.0f};
        float fadeSeconds = 0.5f;
        float holdSeconds = 5.0f;
    };

    Impl() : host_(kDocumentPath) {}
    ~Impl() { Release(); }

    bool Load(bool show)
    {
        if (host_.IsLoaded())
        {
            if (show)
                return Show();
            return true;
        }
        if (!host_.Load(show))
            return false;

        document_ = host_.GetDocument();
        if (!document_)
            return false;

        const auto values = LoadDesignValues();
        (void)ParsePair(values, "Panel-Size", design_.panelSize);
        (void)ParseScalar(values, "Title-FadeSeconds", design_.fadeSeconds);
        (void)ParseScalar(values, "Title-HoldSeconds", design_.holdSeconds);

        panel_ = document_->GetElementById("panel");
        content_ = document_->GetElementById("content");
        strife_ = document_->GetElementById("tfStrife");
        mapName_ = document_->GetElementById("tfMapName");
        occupyGuild_ = document_->GetElementById("tfOccupyGuild");
        if (!panel_ || !content_ || !strife_ || !mapName_ || !occupyGuild_)
        {
            Release();
            return false;
        }

        hostVisible_ = show;
        panel_->SetProperty("display", "none");
        ApplyViewport();
        return true;
    }

    bool Show()
    {
        if (!host_.IsLoaded() && !Load(false))
            return false;
        hostVisible_ = true;
        return host_.Show();
    }

    bool Hide()
    {
        active_ = false;
        hostVisible_ = false;
        if (panel_)
            panel_->SetProperty("display", "none");
        return host_.Hide();
    }

    void Release()
    {
        panel_ = content_ = strife_ = mapName_ = occupyGuild_ = nullptr;
        document_ = nullptr;
        active_ = false;
        host_.Release();
    }

    bool IsLoaded() const noexcept
    {
        return host_.IsLoaded() && document_ != nullptr;
    }

    void SetViewport(int width, int height) noexcept
    {
        viewportWidth_ = std::max(0, width);
        viewportHeight_ = std::max(0, height);
        if (IsLoaded())
            ApplyViewport();
    }

    bool ShowMapName(
        std::string mapName,
        std::string occupyGuild,
        bool strife)
    {
        if (!IsLoaded() && !Load(true))
            return false;
        if (!hostVisible_)
            (void)Show();

        currentMapName_ = std::move(mapName);
        currentGuild_ = std::move(occupyGuild);
        currentStrife_ = strife;
        started_ = std::chrono::steady_clock::now();
        active_ = true;

        SetText(mapName_, currentMapName_);
        SetText(occupyGuild_, currentGuild_);
        SetText(strife_, currentStrife_ ? "Strife" : "");
        strife_->SetProperty(
            "display", currentStrife_ ? "block" : "none");
        occupyGuild_->SetProperty(
            "display", currentGuild_.empty() ? "none" : "block");
        panel_->SetProperty("display", "block");
        panel_->SetProperty("opacity", "0");
        ApplyViewport();
        document_->UpdateDocument();
        return true;
    }

    bool Update()
    {
        if (!active_ || !IsLoaded())
            return false;

        const double elapsed =
            std::chrono::duration<double>(
                std::chrono::steady_clock::now() - started_).count();
        const double fade = std::max(0.001, static_cast<double>(design_.fadeSeconds));
        const double hold = std::max(0.0, static_cast<double>(design_.holdSeconds));

        float alpha = 0.0f;
        if (elapsed < fade)
            alpha = static_cast<float>(elapsed / fade);
        else if (elapsed < fade + hold)
            alpha = 1.0f;
        else if (elapsed < fade + hold + fade)
            alpha = 1.0f - static_cast<float>((elapsed - fade - hold) / fade);
        else
        {
            active_ = false;
            panel_->SetProperty("display", "none");
            return true;
        }

        panel_->SetProperty("opacity", ScalarValue(std::clamp(alpha, 0.0f, 1.0f)));
        return true;
    }

private:
    void ApplyViewport()
    {
        if (!panel_)
            return;
        panel_->SetProperty("width", PixelValue(design_.panelSize[0]));
        panel_->SetProperty("height", PixelValue(design_.panelSize[1]));

        // Public RCSS anchors the panel at 50% x / 45.8333% y. Use pixels
        // here as well so behavior remains stable under reconstructed scaling.
        if (viewportWidth_ > 0)
            panel_->SetProperty(
                "left",
                PixelValue(
                    (static_cast<float>(viewportWidth_) -
                     design_.panelSize[0]) * 0.5f));
        if (viewportHeight_ > 0)
            panel_->SetProperty(
                "top",
                PixelValue(
                    static_cast<float>(viewportHeight_) *
                    (220.0f / 480.0f)));
    }

    RmlDocumentHost host_;
    Rml::ElementDocument* document_ = nullptr;
    Rml::Element* panel_ = nullptr;
    Rml::Element* content_ = nullptr;
    Rml::Element* strife_ = nullptr;
    Rml::Element* mapName_ = nullptr;
    Rml::Element* occupyGuild_ = nullptr;

    Design design_;
    int viewportWidth_ = 0;
    int viewportHeight_ = 0;
    bool hostVisible_ = false;
    bool active_ = false;
    bool currentStrife_ = false;
    std::string currentMapName_;
    std::string currentGuild_;
    std::chrono::steady_clock::time_point started_{};
};

RmlMapNameLayer::RmlMapNameLayer()
    : m_impl(std::make_unique<Impl>()) {}
RmlMapNameLayer::~RmlMapNameLayer() = default;
RmlMapNameLayer::RmlMapNameLayer(RmlMapNameLayer&&) noexcept = default;
RmlMapNameLayer& RmlMapNameLayer::operator=(RmlMapNameLayer&&) noexcept = default;

bool RmlMapNameLayer::Load(bool show) { return m_impl && m_impl->Load(show); }
bool RmlMapNameLayer::Show() { return m_impl && m_impl->Show(); }
bool RmlMapNameLayer::Hide() { return m_impl && m_impl->Hide(); }
void RmlMapNameLayer::Release() { if (m_impl) m_impl->Release(); }
bool RmlMapNameLayer::IsLoaded() const noexcept { return m_impl && m_impl->IsLoaded(); }
void RmlMapNameLayer::SetViewport(int width, int height) noexcept { if (m_impl) m_impl->SetViewport(width, height); }
bool RmlMapNameLayer::ShowMapName(std::string mapName, std::string occupyGuild, bool strife) { return m_impl && m_impl->ShowMapName(std::move(mapName), std::move(occupyGuild), strife); }
bool RmlMapNameLayer::Update() { return m_impl && m_impl->Update(); }

} // namespace UI::Modern::PC::World

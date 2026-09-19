#include "stdafx.h"
#include "UI/Modern/PC/HUD/RmlMiniMapLayer.h"

#include "UI/Modern/RmlDocumentHost.h"
#include "UI/Modern/RmlHudMapViewport.h"
#include "UI/Modern/RmlUiDesign.h"

#include <RmlUi/Core/Core.h>
#include <RmlUi/Core/Element.h>
#include <RmlUi/Core/ElementDocument.h>
#include <RmlUi/Core/FileInterface.h>
#include <RmlUi/Core/StreamMemory.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <sstream>
#include <utility>

namespace UI::Modern::PC::HUD
{

namespace
{
constexpr const char* kDocumentPath = "Data/UI/PC/HUD/minimap.rml";

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
    std::array<char, 64> buffer{};
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

template <std::size_t N>
bool ParseArray(
    const RmlUiDesign::Values& values,
    const char* key,
    std::array<float, N>& output)
{
    const auto it = values.find(key);
    if (it == values.end())
        return false;
    std::istringstream stream(it->second);
    std::array<float, N> parsed{};
    for (float& value : parsed)
        if (!(stream >> value))
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

void SetText(Rml::Element* element, const std::string& text)
{
    if (!element)
        return;
    element->SetInnerRML("");
    if (Rml::ElementDocument* document = element->GetOwnerDocument())
        element->AppendChild(document->CreateTextNode(text));
}
} // namespace

class RmlMiniMapLayer::Impl
{
public:
    struct Design
    {
        float initialScaleX = 2.25f;
        float initialScaleY = 1.5f;
        float initialAlpha = 55.0f;
        float centerYOffset = 30.0f;
        float helpDelayMs = 6000.0f;
        float helpFadeSeconds = 1.6f;
        float smallScale = 0.83f;
        std::array<float, 2> maskSize{800.0f, 600.0f};
        std::array<float, 4> rotation{
            0.7070770264f, 0.7070617676f,
           -0.7070617676f, 0.7070770264f};
        float frameRate = 24.0f;
        float alphaStep = 25.0f;
        float alphaMin = 25.0f;
        float alphaMax = 85.0f;
        float scaleStep = 25.0f;
        float scaleMin = 125.0f;
        float scaleMax = 350.0f;
        float imageAlphaFactor = 0.7f;
        float iconAlphaOffset = 15.0f;
        float smallStageHeight = 700.0f;
        float nameScale = 150.0f;
        std::array<float, 2> iconRotation{-45.0f, -45.0f};
        float labelExtra = 5.0f;
        int heroFrames = 30;
        std::array<float, 2> portalGrid{28.25f, 81.25f};
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

        ReadDesign();

        root_ = document_->GetElementById("map-root");
        hostElement_ = document_->GetElementById("map-host");
        hero_ = document_->GetElementById("map-hero");
        markers_ = document_->GetElementById("map-markers");
        help_ = document_->GetElementById("map-help");
        mapView_ = dynamic_cast<UI::Modern::RmlHudMapViewport*>(
            document_->GetElementById("map-view"));

        if (!root_ || !hostElement_ || !hero_ || !markers_ ||
            !help_ || !mapView_)
        {
            Release();
            return false;
        }

        hostVisible_ = show;
        return ApplyState(std::move(state_));
    }

    bool Show()
    {
        if (!host_.IsLoaded() && !Load(false))
            return false;
        hostVisible_ = true;
        const bool result = host_.Show();
        if (result)
            (void)ApplyState(std::move(state_));
        return result;
    }

    bool Hide()
    {
        hostVisible_ = false;
        if (root_)
            root_->SetProperty("display", "none");
        return host_.Hide();
    }

    void Release()
    {
        mapView_ = nullptr;
        root_ = hostElement_ = hero_ = markers_ = help_ = nullptr;
        document_ = nullptr;
        host_.Release();
    }

    bool IsLoaded() const noexcept
    {
        return host_.IsLoaded() && document_ != nullptr;
    }

    bool SetMapImage(
        const mu::pipeline::LogicalRenderAssetMetadata& metadata)
    {
        return mapView_ && mapView_->SetImage(metadata);
    }

    bool ApplyState(State state)
    {
        state_ = std::move(state);
        if (!IsLoaded())
            return false;

        const bool visible =
            hostVisible_ && state_.visible &&
            state_.viewportWidth > 0 && state_.viewportHeight > 0;
        root_->SetProperty("display", visible ? "block" : "none");
        if (!visible)
            return true;

        const float stageScale =
            state_.viewportHeight < design_.smallStageHeight
                ? design_.smallScale
                : 1.0f;

        const float centerX =
            static_cast<float>(state_.viewportWidth) * 0.5f;
        const float centerY =
            static_cast<float>(state_.viewportHeight) * 0.5f +
            design_.centerYOffset * stageScale;

        const float mapWidth = std::max(1.0f, state_.mapWidth * stageScale);
        const float mapHeight = std::max(1.0f, state_.mapHeight * stageScale);

        // Legacy map storage and the recovered MuClient minimap use the
        // historical MU X/Y orientation: screen-local X derives from world Y.
        const float heroMapX =
            std::clamp(state_.heroWorldY / 256.0f, 0.0f, 1.0f) * mapWidth;
        const float heroMapY =
            std::clamp(state_.heroWorldX / 256.0f, 0.0f, 1.0f) * mapHeight;

        mapView_->SetProperty("display", "block");
        mapView_->SetProperty("left", PixelValue(centerX - heroMapX));
        mapView_->SetProperty("top", PixelValue(centerY - heroMapY));
        mapView_->SetProperty("width", PixelValue(mapWidth));
        mapView_->SetProperty("height", PixelValue(mapHeight));
        mapView_->SetProperty(
            "transform-origin",
            PixelValue(heroMapX) + " " + PixelValue(heroMapY));
        mapView_->SetProperty("transform", "rotate(45deg)");

        const float alpha = std::clamp(
            state_.alphaPercent, design_.alphaMin, design_.alphaMax);
        mapView_->SetProperty(
            "opacity",
            ScalarValue(
                (alpha / 100.0f) * design_.imageAlphaFactor));

        hero_->SetProperty("display", "block");
        hero_->SetProperty("left", PixelValue(centerX));
        hero_->SetProperty("top", PixelValue(centerY));
        hero_->SetProperty(
            "opacity",
            ScalarValue(std::clamp(
                (alpha + design_.iconAlphaOffset) / 100.0f,
                0.0f, 1.0f)));

        markers_->SetInnerRML("");
        for (std::size_t i = 0; i < state_.markers.size(); ++i)
        {
            const Marker& marker = state_.markers[i];

            const float markerMapX =
                std::clamp(marker.worldY / 256.0f, 0.0f, 1.0f) * mapWidth;
            const float markerMapY =
                std::clamp(marker.worldX / 256.0f, 0.0f, 1.0f) * mapHeight;
            const float dx = markerMapX - heroMapX;
            const float dy = markerMapY - heroMapY;

            const float screenX =
                centerX + design_.rotation[0] * dx +
                design_.rotation[1] * dy;
            const float screenY =
                centerY + design_.rotation[2] * dx +
                design_.rotation[3] * dy;

            Rml::ElementPtr item = document_->CreateElement("div");
            item->SetId(
                Rml::String("map-marker-") + std::to_string(i));
            item->SetClass("map-marker", true);
            item->SetClass(
                marker.kind == MarkerKind::Portal ? "portal" : "npc",
                true);
            item->SetProperty("left", PixelValue(screenX));
            item->SetProperty("top", PixelValue(screenY));
            item->SetProperty(
                "opacity",
                ScalarValue(std::clamp(
                    (alpha + design_.iconAlphaOffset) / 100.0f,
                    0.0f, 1.0f)));

            Rml::ElementPtr icon = document_->CreateElement("div");
            icon->SetClass("map-marker-icon", true);
            if (marker.kind == MarkerKind::Portal)
                icon->SetProperty(
                    "transform",
                    Rml::String("rotate(") +
                    std::to_string(marker.rotationDegrees) + "deg)");
            item->AppendChild(std::move(icon));

            if (!marker.name.empty())
            {
                Rml::ElementPtr label = document_->CreateElement("div");
                label->SetClass("map-marker-label", true);
                label->AppendChild(document_->CreateTextNode(marker.name));
                item->AppendChild(std::move(label));
            }

            markers_->AppendChild(std::move(item));
        }

        help_->SetProperty(
            "opacity",
            ScalarValue(std::clamp(alpha / 100.0f, 0.0f, 1.0f)));

        document_->UpdateDocument();
        return true;
    }

    void RequestAlphaStep(int direction) noexcept
    {
        if (direction == 0)
            return;
        alphaStepRequest_ = direction < 0 ? -1 : 1;
    }

    void RequestScaleStep(int direction) noexcept
    {
        if (direction == 0)
            return;
        scaleStepRequest_ = direction < 0 ? -1 : 1;
    }

    void RequestClose() noexcept { closeRequest_ = true; }

    std::optional<int> ConsumeAlphaStep() noexcept
    {
        return std::exchange(alphaStepRequest_, std::nullopt);
    }

    std::optional<int> ConsumeScaleStep() noexcept
    {
        return std::exchange(scaleStepRequest_, std::nullopt);
    }

    bool ConsumeCloseRequest() noexcept
    {
        return std::exchange(closeRequest_, false);
    }

private:
    void ReadDesign()
    {
        const auto values = LoadDesignValues();
        (void)ParseScalar(values, "Map-InitialScaleX", design_.initialScaleX);
        (void)ParseScalar(values, "Map-InitialScaleY", design_.initialScaleY);
        (void)ParseScalar(values, "Map-InitialAlpha", design_.initialAlpha);
        (void)ParseScalar(values, "Map-CenterYOffset", design_.centerYOffset);
        (void)ParseScalar(values, "Map-HelpDelayMs", design_.helpDelayMs);
        (void)ParseScalar(values, "Map-HelpFadeSeconds", design_.helpFadeSeconds);
        (void)ParseScalar(values, "Map-SmallScale", design_.smallScale);
        (void)ParseArray(values, "Map-MaskSize", design_.maskSize);
        (void)ParseArray(values, "Map-RotationMatrix", design_.rotation);
        (void)ParseScalar(values, "Map-FrameRate", design_.frameRate);
        (void)ParseScalar(values, "Map-AlphaStep", design_.alphaStep);
        (void)ParseScalar(values, "Map-AlphaMin", design_.alphaMin);
        (void)ParseScalar(values, "Map-AlphaMax", design_.alphaMax);
        (void)ParseScalar(values, "Map-ScaleStep", design_.scaleStep);
        (void)ParseScalar(values, "Map-ScaleMin", design_.scaleMin);
        (void)ParseScalar(values, "Map-ScaleMax", design_.scaleMax);
        (void)ParseScalar(values, "Map-ImageAlphaFactor", design_.imageAlphaFactor);
        (void)ParseScalar(values, "Map-IconAlphaOffset", design_.iconAlphaOffset);
        (void)ParseScalar(values, "Map-SmallStageHeight", design_.smallStageHeight);
        (void)ParseScalar(values, "Map-NameScale", design_.nameScale);
        (void)ParseArray(values, "Map-IconRotation", design_.iconRotation);
        (void)ParseScalar(values, "Map-LabelExtra", design_.labelExtra);
        (void)ParseScalar(values, "Map-HeroFrames", design_.heroFrames);
        (void)ParseArray(values, "Map-PortalGrid", design_.portalGrid);
    }

    RmlDocumentHost host_;
    Rml::ElementDocument* document_ = nullptr;
    Rml::Element* root_ = nullptr;
    Rml::Element* hostElement_ = nullptr;
    UI::Modern::RmlHudMapViewport* mapView_ = nullptr;
    Rml::Element* hero_ = nullptr;
    Rml::Element* markers_ = nullptr;
    Rml::Element* help_ = nullptr;

    bool hostVisible_ = false;
    Design design_;
    State state_;
    std::optional<int> alphaStepRequest_;
    std::optional<int> scaleStepRequest_;
    bool closeRequest_ = false;
};

RmlMiniMapLayer::RmlMiniMapLayer()
    : m_impl(std::make_unique<Impl>()) {}
RmlMiniMapLayer::~RmlMiniMapLayer() = default;
RmlMiniMapLayer::RmlMiniMapLayer(RmlMiniMapLayer&&) noexcept = default;
RmlMiniMapLayer& RmlMiniMapLayer::operator=(
    RmlMiniMapLayer&&) noexcept = default;

bool RmlMiniMapLayer::Load(bool show) { return m_impl && m_impl->Load(show); }
bool RmlMiniMapLayer::Show() { return m_impl && m_impl->Show(); }
bool RmlMiniMapLayer::Hide() { return m_impl && m_impl->Hide(); }
void RmlMiniMapLayer::Release() { if (m_impl) m_impl->Release(); }
bool RmlMiniMapLayer::IsLoaded() const noexcept { return m_impl && m_impl->IsLoaded(); }
bool RmlMiniMapLayer::SetMapImage(const mu::pipeline::LogicalRenderAssetMetadata& metadata) { return m_impl && m_impl->SetMapImage(metadata); }
bool RmlMiniMapLayer::ApplyState(State state) { return m_impl && m_impl->ApplyState(std::move(state)); }
void RmlMiniMapLayer::RequestAlphaStep(int direction) noexcept { if (m_impl) m_impl->RequestAlphaStep(direction); }
void RmlMiniMapLayer::RequestScaleStep(int direction) noexcept { if (m_impl) m_impl->RequestScaleStep(direction); }
void RmlMiniMapLayer::RequestClose() noexcept { if (m_impl) m_impl->RequestClose(); }
std::optional<int> RmlMiniMapLayer::ConsumeAlphaStep() noexcept { return m_impl ? m_impl->ConsumeAlphaStep() : std::nullopt; }
std::optional<int> RmlMiniMapLayer::ConsumeScaleStep() noexcept { return m_impl ? m_impl->ConsumeScaleStep() : std::nullopt; }
bool RmlMiniMapLayer::ConsumeCloseRequest() noexcept { return m_impl && m_impl->ConsumeCloseRequest(); }

} // namespace UI::Modern::PC::HUD

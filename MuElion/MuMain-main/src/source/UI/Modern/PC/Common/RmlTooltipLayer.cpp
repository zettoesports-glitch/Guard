#include "stdafx.h"
#include "UI/Modern/PC/Common/RmlTooltipLayer.h"

#include "UI/Modern/RmlDocumentHost.h"
#include "UI/Modern/RmlUiDesign.h"

#include <RmlUi/Core/Core.h>
#include <RmlUi/Core/Element.h>
#include <RmlUi/Core/ElementDocument.h>
#include <RmlUi/Core/FileInterface.h>
#include <RmlUi/Core/StreamMemory.h>
#include <RmlUi/Core/StringUtilities.h>

#include <algorithm>
#include <array>
#include <cstdio>
#include <sstream>
#include <string>
#include <utility>

namespace UI::Modern::PC::Common
{

namespace
{
constexpr const char* kDocumentPath =
    "Data/UI/PC/Common/tooltip.rml";

Rml::String PixelValue(float value)
{
    std::array<char, 64> buffer{};
    std::snprintf(
        buffer.data(), buffer.size(), "%fpx",
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

const char* ColorClass(
    RmlTooltipLayer::TextColor color) noexcept
{
    using C = RmlTooltipLayer::TextColor;
    switch (color)
    {
    case C::Blue: return " blue";
    case C::Gray: return " gray";
    case C::Red: return " red";
    case C::Yellow: return " yellow";
    case C::Green: return " green";
    case C::Purple: return " purple";
    case C::RedPurple: return " red-purple";
    case C::Violet: return " violet";
    case C::Orange: return " orange";
    case C::Default:
    default:
        return "";
    }
}

const char* BackgroundClass(
    RmlTooltipLayer::Background background) noexcept
{
    using B = RmlTooltipLayer::Background;
    switch (background)
    {
    case B::DarkRed: return " dark-red";
    case B::DarkBlue: return " dark-blue";
    case B::DarkYellow: return " dark-yellow";
    case B::GreenBlue: return " green-blue";
    case B::None:
    default:
        return "";
    }
}

const char* AlignmentValue(
    RmlTooltipLayer::Alignment alignment) noexcept
{
    // Debug state: 0 -> left, 2 -> right, everything else -> center.
    switch (alignment)
    {
    case RmlTooltipLayer::Alignment::Left:
        return "left";
    case RmlTooltipLayer::Alignment::Right:
        return "right";
    case RmlTooltipLayer::Alignment::Center:
    default:
        return "center";
    }
}

bool IsBlankLine(const std::string& text) noexcept
{
    // Exact recovered special cases are a leading newline, or one single
    // space followed by NUL.
    return (!text.empty() && text.front() == '\n') || text == " ";
}

Rml::String BuildLineRml(const RmlTooltipLayer::Line& line)
{
    Rml::String result = "<div class=\"tooltip-line";
    const bool blank = IsBlankLine(line.text);

    if (blank)
    {
        result += " blank";
    }
    else
    {
        result += ColorClass(line.color);
        result += BackgroundClass(line.background);
    }

    if (line.bold)
        result += " bold";

    result += "\">";

    if (!blank)
        result += Rml::StringUtilities::EncodeRml(line.text);

    result += "</div>";
    return result;
}

Rml::String BuildTooltipRml(
    const std::vector<RmlTooltipLayer::Line>& lines)
{
    Rml::String result;
    for (const auto& line : lines)
        result += BuildLineRml(line);
    return result;
}

} // namespace

class RmlTooltipLayer::Impl
{
public:
    struct Design
    {
        std::size_t capacity = 8;
        std::size_t minimumContentSize = 1;
    };

    Impl()
        : host_(kDocumentPath)
    {
    }

    ~Impl()
    {
        Release();
    }

    [[nodiscard]] bool Load(bool show)
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

        elements_.assign(design_.capacity, nullptr);
        for (std::size_t index = 0;
             index < design_.capacity;
             ++index)
        {
            std::array<char, 64> id{};
            std::snprintf(
                id.data(), id.size(), "tooltip-%d",
                static_cast<int>(index));

            elements_[index] =
                document_->GetElementById(id.data());
            if (!elements_[index])
            {
                Release();
                return false;
            }
        }

        bound_ = true;
        return true;
    }

    [[nodiscard]] bool Show()
    {
        if (!host_.IsLoaded() && !Load(false))
            return false;
        return host_.Show();
    }

    [[nodiscard]] bool Hide()
    {
        return host_.Hide();
    }

    void Release()
    {
        elements_.clear();
        document_ = nullptr;
        bound_ = false;
        host_.Release();
    }

    [[nodiscard]] bool IsLoaded() const noexcept
    {
        return bound_ && document_ != nullptr && host_.IsLoaded();
    }

    [[nodiscard]] std::size_t Capacity() const noexcept
    {
        return elements_.size();
    }

    [[nodiscard]] bool SetTooltip(
        std::size_t index,
        const Tooltip& tooltip)
    {
        if (!IsLoaded() || index >= elements_.size())
            return false;

        Rml::Element* element = elements_[index];
        if (!element)
            return false;

        if (!tooltip.visible)
        {
            element->SetProperty("display", "none");
            return true;
        }

        element->SetProperty("display", "block");
        element->SetProperty("left", PixelValue(tooltip.x));
        element->SetProperty("top", PixelValue(tooltip.y));

        const float minimumWidth = std::max(
            tooltip.minimumWidth,
            static_cast<float>(design_.minimumContentSize));
        element->SetProperty(
            "min-width", PixelValue(minimumWidth));

        element->SetProperty(
            "text-align", AlignmentValue(tooltip.alignment));
        element->SetClass("framed", tooltip.framed);

        element->SetInnerRML(BuildTooltipRml(tooltip.lines));
        return true;
    }

    [[nodiscard]] bool HideTooltip(std::size_t index)
    {
        if (!IsLoaded() || index >= elements_.size())
            return false;

        if (Rml::Element* element = elements_[index])
        {
            element->SetProperty("display", "none");
            return true;
        }

        return false;
    }

    void HideAll()
    {
        for (Rml::Element* element : elements_)
            if (element)
                element->SetProperty("display", "none");
    }

private:
    void ReadDesign()
    {
        const RmlUiDesign::Values values = LoadDesignValues();
        (void)ParseScalar(
            values, "RmlTooltipLayer-TooltipCapacity",
            design_.capacity);
        (void)ParseScalar(
            values, "RmlTooltipLayer-MinimumContentSize",
            design_.minimumContentSize);

        design_.capacity = std::max<std::size_t>(
            1u, design_.capacity);
        design_.minimumContentSize =
            std::max<std::size_t>(
                1u, design_.minimumContentSize);
    }

    RmlDocumentHost host_;
    Rml::ElementDocument* document_ = nullptr;
    Design design_;
    std::vector<Rml::Element*> elements_;
    bool bound_ = false;
};

RmlTooltipLayer::RmlTooltipLayer()
    : m_impl(std::make_unique<Impl>())
{
}

RmlTooltipLayer::~RmlTooltipLayer() = default;
RmlTooltipLayer::RmlTooltipLayer(
    RmlTooltipLayer&&) noexcept = default;
RmlTooltipLayer& RmlTooltipLayer::operator=(
    RmlTooltipLayer&&) noexcept = default;

bool RmlTooltipLayer::Load(bool show)
{
    return m_impl && m_impl->Load(show);
}

bool RmlTooltipLayer::Show()
{
    return m_impl && m_impl->Show();
}

bool RmlTooltipLayer::Hide()
{
    return m_impl && m_impl->Hide();
}

void RmlTooltipLayer::Release()
{
    if (m_impl)
        m_impl->Release();
}

bool RmlTooltipLayer::IsLoaded() const noexcept
{
    return m_impl && m_impl->IsLoaded();
}

std::size_t RmlTooltipLayer::Capacity() const noexcept
{
    return m_impl ? m_impl->Capacity() : 0u;
}

bool RmlTooltipLayer::SetTooltip(
    std::size_t index,
    const Tooltip& tooltip)
{
    return m_impl &&
        m_impl->SetTooltip(index, tooltip);
}

bool RmlTooltipLayer::HideTooltip(std::size_t index)
{
    return m_impl &&
        m_impl->HideTooltip(index);
}

void RmlTooltipLayer::HideAll()
{
    if (m_impl)
        m_impl->HideAll();
}

} // namespace UI::Modern::PC::Common

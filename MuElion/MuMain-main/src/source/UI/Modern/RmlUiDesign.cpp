#include "stdafx.h"
#include "UI/Modern/RmlUiDesign.h"

#include <RmlUi/Core/Dictionary.h>
#include <RmlUi/Core/Stream.h>

#include <string_view>

namespace UI::Modern
{

namespace
{
constexpr std::string_view kMetaTag = "meta";
constexpr std::string_view kNameAttribute = "name";
constexpr std::string_view kContentAttribute = "content";
constexpr std::string_view kDesignPrefix = "mu-design-";
}

void RmlUiDesign::Parser::HandleElementStart(
    const Rml::String& name,
    const Rml::XMLAttributes& attributes)
{
    // x64 Debug disassembly compares the element name against "meta", reads
    // "name", requires the exact "mu-design-" prefix, strips that prefix, and
    // stores the "content" attribute in the parser-owned string map.
    if (name != kMetaTag)
        return;

    const Rml::String designName = Rml::Get(
        attributes, Rml::String(kNameAttribute), Rml::String{});
    if (!designName.starts_with(kDesignPrefix))
        return;

    const Rml::String content = Rml::Get(
        attributes, Rml::String(kContentAttribute), Rml::String{});

    const Rml::String key =
        designName.substr(kDesignPrefix.size());
    values_.insert_or_assign(key, content);
}

void RmlUiDesign::Parser::HandleElementEnd(const Rml::String&)
{
    // The recovered virtual body is empty.
}

void RmlUiDesign::Parser::HandleData(
    const Rml::String&,
    Rml::XMLDataType)
{
    // The recovered virtual body is empty.
}

RmlUiDesign::Values RmlUiDesign::Parse(Rml::Stream* stream)
{
    if (!stream)
        return {};

    Parser parser;
    parser.Rml::BaseXMLParser::Parse(stream);
    return parser.TakeValues();
}

} // namespace UI::Modern

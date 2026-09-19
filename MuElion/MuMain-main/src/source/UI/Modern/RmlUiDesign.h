#pragma once

#include <RmlUi/Core/BaseXMLParser.h>

#include <unordered_map>

namespace UI::Modern
{

// Recovered MuTwo design-metadata parser. RML documents can publish arbitrary
// design constants through:
//   <meta name="mu-design-<key>" content="<value>"/>
// The Debug executable exposes the nested Parser RTTI and the exact prefix.
class RmlUiDesign
{
public:
    using Values = std::unordered_map<Rml::String, Rml::String>;

    class Parser final : public Rml::BaseXMLParser
    {
    public:
        Parser() = default;
        ~Parser() override = default;

        [[nodiscard]] const Values& GetValues() const noexcept
        {
            return values_;
        }

        [[nodiscard]] Values TakeValues() noexcept
        {
            return std::move(values_);
        }

        void HandleElementStart(
            const Rml::String& name,
            const Rml::XMLAttributes& attributes) override;

        void HandleElementEnd(const Rml::String& name) override;
        void HandleData(
            const Rml::String& data,
            Rml::XMLDataType type) override;

    private:
        Values values_;
    };

    [[nodiscard]] static Values Parse(Rml::Stream* stream);
};

} // namespace UI::Modern

#pragma once

#include <RmlUi/Core/Decorator.h>
#include <RmlUi/Core/ID.h>

namespace UI::Modern
{

// Reconstructed from Main-x64-Debug.exe RTTI and property-registration code.
// The nested SpriteDecorator name is intentionally preserved because it is
// present in the Debug executable's MSVC RTTI.
class RmlGfxTintDecoratorInstancer final : public Rml::DecoratorInstancer
{
public:
    RmlGfxTintDecoratorInstancer();
    ~RmlGfxTintDecoratorInstancer() override = default;

    Rml::SharedPtr<Rml::Decorator> InstanceDecorator(
        const Rml::String& name,
        const Rml::PropertyDictionary& properties,
        const Rml::DecoratorInstancerInterface& instancerInterface) override;

private:
    class SpriteDecorator;

    struct PropertyIds
    {
        Rml::PropertyId sprite = Rml::PropertyId::Invalid;
        Rml::PropertyId scale = Rml::PropertyId::Invalid;
        Rml::PropertyId red = Rml::PropertyId::Invalid;
        Rml::PropertyId green = Rml::PropertyId::Invalid;
        Rml::PropertyId blue = Rml::PropertyId::Invalid;
    };

    PropertyIds ids_{};
};

} // namespace UI::Modern

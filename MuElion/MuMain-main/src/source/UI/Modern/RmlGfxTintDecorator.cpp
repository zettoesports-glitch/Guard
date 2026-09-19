#include "stdafx.h"
#include "UI/Modern/RmlGfxTintDecorator.h"

#include <RmlUi/Core/CompiledFilterShader.h>
#include <RmlUi/Core/ComputedValues.h>
#include <RmlUi/Core/Element.h>
#include <RmlUi/Core/Geometry.h>
#include <RmlUi/Core/Mesh.h>
#include <RmlUi/Core/MeshUtilities.h>
#include <RmlUi/Core/RenderManager.h>
#include <RmlUi/Core/Spritesheet.h>
#include <RmlUi/Core/Texture.h>
#include <RmlUi/Core/Variant.h>

#include <utility>

namespace UI::Modern
{

namespace
{
struct GfxTintElementData
{
    GfxTintElementData(Rml::Geometry&& inGeometry,
                       Rml::CompiledShader&& inShader)
        : geometry(std::move(inGeometry)),
          shader(std::move(inShader))
    {
    }

    Rml::Geometry geometry;
    Rml::CompiledShader shader;
};
} // namespace

class RmlGfxTintDecoratorInstancer::SpriteDecorator final
    : public Rml::Decorator
{
public:
    SpriteDecorator(Rml::Texture texture,
                    Rml::Rectanglef rectangle,
                    Rml::Vector4f tint) noexcept
        : texture_(texture),
          rectangle_(rectangle),
          tint_(tint)
    {
    }

    Rml::DecoratorDataHandle GenerateElementData(
        Rml::Element* element,
        Rml::BoxArea paintArea) const override
    {
        if (!element || !texture_)
            return INVALID_DECORATORDATAHANDLE;

        Rml::RenderManager* renderManager = element->GetRenderManager();
        if (!renderManager)
            return INVALID_DECORATORDATAHANDLE;

        const Rml::Vector2i textureDimensions = texture_.GetDimensions();
        if (textureDimensions.x <= 0 || textureDimensions.y <= 0)
            return INVALID_DECORATORDATAHANDLE;

        const Rml::RenderBox renderBox = element->GetRenderBox(paintArea);
        const Rml::Vector2f offset = renderBox.GetFillOffset();
        const Rml::Vector2f size = renderBox.GetFillSize();
        if (size.x <= 0.0f || size.y <= 0.0f)
            return INVALID_DECORATORDATAHANDLE;

        const Rml::Vector2f spritePosition = rectangle_.Position();
        const Rml::Vector2f spriteSize = rectangle_.Size();
        const Rml::Vector2f uv0{
            spritePosition.x / static_cast<float>(textureDimensions.x),
            spritePosition.y / static_cast<float>(textureDimensions.y)
        };
        const Rml::Vector2f uv1{
            (spritePosition.x + spriteSize.x) /
                static_cast<float>(textureDimensions.x),
            (spritePosition.y + spriteSize.y) /
                static_cast<float>(textureDimensions.y)
        };

        const Rml::ComputedValues& computed = element->GetComputedValues();
        const Rml::byte alpha = static_cast<Rml::byte>(
            computed.opacity() * 255.0f);
        const Rml::ColourbPremultiplied colour(alpha, alpha);

        Rml::Mesh mesh;
        Rml::MeshUtilities::GenerateQuad(
            mesh, offset, size, colour, uv0, uv1);

        Rml::CompiledShader shader = renderManager->CompileShader(
            "gfx-tint",
            Rml::Dictionary{{"tint", Rml::Variant(tint_)}});
        if (!shader)
            return INVALID_DECORATORDATAHANDLE;

        Rml::Geometry geometry =
            renderManager->MakeGeometry(std::move(mesh));
        if (!geometry)
            return INVALID_DECORATORDATAHANDLE;

        return reinterpret_cast<Rml::DecoratorDataHandle>(
            new GfxTintElementData(
                std::move(geometry), std::move(shader)));
    }

    void ReleaseElementData(
        Rml::DecoratorDataHandle elementData) const override
    {
        delete reinterpret_cast<GfxTintElementData*>(elementData);
    }

    void RenderElement(
        Rml::Element* element,
        Rml::DecoratorDataHandle elementData) const override
    {
        if (!element || elementData == INVALID_DECORATORDATAHANDLE)
            return;

        auto* data =
            reinterpret_cast<GfxTintElementData*>(elementData);
        data->geometry.Render(
            element->GetAbsoluteOffset(Rml::BoxArea::Border),
            texture_,
            data->shader);
    }

private:
    Rml::Texture texture_;
    Rml::Rectanglef rectangle_;
    Rml::Vector4f tint_{1.0f, 0.0f, 0.0f, 0.0f};
};

RmlGfxTintDecoratorInstancer::RmlGfxTintDecoratorInstancer()
{
    ids_.sprite =
        RegisterProperty("sprite", "").AddParser("string").GetId();
    ids_.scale =
        RegisterProperty("scale", "1").AddParser("number").GetId();
    ids_.red =
        RegisterProperty("red", "0").AddParser("number").GetId();
    ids_.green =
        RegisterProperty("green", "0").AddParser("number").GetId();
    ids_.blue =
        RegisterProperty("blue", "0").AddParser("number").GetId();

    RegisterShorthand(
        "decorator",
        "sprite, scale, red, green, blue",
        Rml::ShorthandType::FallThrough);
}

Rml::SharedPtr<Rml::Decorator>
RmlGfxTintDecoratorInstancer::InstanceDecorator(
    const Rml::String&,
    const Rml::PropertyDictionary& properties,
    const Rml::DecoratorInstancerInterface& instancerInterface)
{
    const Rml::Property* spriteProperty =
        properties.GetProperty(ids_.sprite);
    const Rml::Property* scaleProperty =
        properties.GetProperty(ids_.scale);
    const Rml::Property* redProperty =
        properties.GetProperty(ids_.red);
    const Rml::Property* greenProperty =
        properties.GetProperty(ids_.green);
    const Rml::Property* blueProperty =
        properties.GetProperty(ids_.blue);

    if (!spriteProperty || !scaleProperty || !redProperty ||
        !greenProperty || !blueProperty)
        return nullptr;

    const Rml::String spriteName =
        spriteProperty->Get<Rml::String>();
    if (spriteName.empty())
        return nullptr;

    const Rml::Sprite* sprite =
        instancerInterface.GetSprite(spriteName);
    if (!sprite || !sprite->sprite_sheet)
        return nullptr;

    Rml::Texture texture =
        sprite->sprite_sheet->texture_source.GetTexture(
            instancerInterface.GetRenderManager());
    if (!texture)
        return nullptr;

    constexpr float kByteToUnit = 1.0f / 255.0f;
    const Rml::Vector4f tint{
        scaleProperty->Get<float>(),
        redProperty->Get<float>() * kByteToUnit,
        greenProperty->Get<float>() * kByteToUnit,
        blueProperty->Get<float>() * kByteToUnit
    };

    return Rml::MakeShared<SpriteDecorator>(
        texture, sprite->rectangle, tint);
}

} // namespace UI::Modern

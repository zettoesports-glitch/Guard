#include "stdafx.h"
#include "UI/Modern/PC/World/RmlMonsterInfoLayer.h"

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
#include <utility>

namespace UI::Modern::PC::World
{

namespace
{
constexpr const char* kDocumentPath =
    "Data/UI/PC/World/monster_info.rml";

Rml::String PixelValue(float value)
{
    std::array<char, 64> buffer{};
    std::snprintf(
        buffer.data(), buffer.size(), "%.3fpx",
        static_cast<double>(value));
    return Rml::String(buffer.data());
}

Rml::String PercentValue(float value)
{
    std::array<char, 32> buffer{};
    std::snprintf(
        buffer.data(), buffer.size(), "%.3f%%",
        static_cast<double>(value * 100.0f));
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
} // namespace

class RmlMonsterInfoLayer::Impl
{
public:
    struct Design
    {
        float worldRaise = 60.0f;
        std::array<float, 2> targetAnchor{320.0f, 24.0f};
        std::array<float, 2> reference{640.0f, 480.0f};
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
        (void)ParseScalar(values, "World-Raise", design_.worldRaise);
        (void)ParsePair(values, "Target-Anchor", design_.targetAnchor);
        (void)ParsePair(values, "Panel-Reference", design_.reference);

        monsters_ = document_->GetElementById("monsters");
        if (!monsters_)
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
        if (monsters_)
            monsters_->SetProperty("display", "none");
        return host_.Hide();
    }

    void Release()
    {
        monsters_ = nullptr;
        document_ = nullptr;
        host_.Release();
    }

    bool IsLoaded() const noexcept
    {
        return host_.IsLoaded() && document_ != nullptr;
    }

    bool ApplyState(State state)
    {
        state_ = std::move(state);
        if (!IsLoaded())
            return false;

        const bool visible =
            hostVisible_ && state_.visible &&
            state_.viewportWidth > 0 && state_.viewportHeight > 0;
        monsters_->SetProperty("display", visible ? "block" : "none");
        if (!visible)
            return true;

        monsters_->SetInnerRML("");

        const float sx = design_.reference[0] > 0.0f
            ? static_cast<float>(state_.viewportWidth) / design_.reference[0]
            : 1.0f;
        const float sy = design_.reference[1] > 0.0f
            ? static_cast<float>(state_.viewportHeight) / design_.reference[1]
            : 1.0f;

        for (std::size_t i = 0; i < state_.monsters.size(); ++i)
        {
            const Monster& monster = state_.monsters[i];
            if (!monster.visible)
                continue;

            const float x = monster.target
                ? design_.targetAnchor[0] * sx
                : monster.screenX;
            const float y = monster.target
                ? design_.targetAnchor[1] * sy
                : monster.screenY - design_.worldRaise * sy;

            Rml::ElementPtr holder = document_->CreateElement("div");
            holder->SetId(
                Rml::String("monster-info-") + std::to_string(i));
            holder->SetClass("monster", true);
            holder->SetClass("target", monster.target);
            holder->SetProperty("left", PixelValue(x));
            holder->SetProperty("top", PixelValue(y));

            Rml::ElementPtr name = document_->CreateElement("div");
            name->SetClass("monster-name", true);
            name->AppendChild(document_->CreateTextNode(monster.name));
            holder->AppendChild(std::move(name));

            Rml::ElementPtr hp = document_->CreateElement("div");
            hp->SetClass("hp", true);

            Rml::ElementPtr background = document_->CreateElement("div");
            background->SetClass("hp-background", true);
            hp->AppendChild(std::move(background));

            Rml::ElementPtr track = document_->CreateElement("div");
            track->SetClass("hp-track", true);

            Rml::ElementPtr fill = document_->CreateElement("div");
            fill->SetClass("hp-fill", true);
            fill->SetProperty(
                "width",
                PercentValue(std::clamp(
                    monster.hpFraction, 0.0f, 1.0f)));
            track->AppendChild(std::move(fill));
            hp->AppendChild(std::move(track));
            holder->AppendChild(std::move(hp));

            monsters_->AppendChild(std::move(holder));
        }

        document_->UpdateDocument();
        return true;
    }

private:
    RmlDocumentHost host_;
    Rml::ElementDocument* document_ = nullptr;
    Rml::Element* monsters_ = nullptr;
    bool hostVisible_ = false;
    Design design_;
    State state_;
};

RmlMonsterInfoLayer::RmlMonsterInfoLayer()
    : m_impl(std::make_unique<Impl>()) {}
RmlMonsterInfoLayer::~RmlMonsterInfoLayer() = default;
RmlMonsterInfoLayer::RmlMonsterInfoLayer(RmlMonsterInfoLayer&&) noexcept = default;
RmlMonsterInfoLayer& RmlMonsterInfoLayer::operator=(RmlMonsterInfoLayer&&) noexcept = default;

bool RmlMonsterInfoLayer::Load(bool show) { return m_impl && m_impl->Load(show); }
bool RmlMonsterInfoLayer::Show() { return m_impl && m_impl->Show(); }
bool RmlMonsterInfoLayer::Hide() { return m_impl && m_impl->Hide(); }
void RmlMonsterInfoLayer::Release() { if (m_impl) m_impl->Release(); }
bool RmlMonsterInfoLayer::IsLoaded() const noexcept { return m_impl && m_impl->IsLoaded(); }
bool RmlMonsterInfoLayer::ApplyState(State state) { return m_impl && m_impl->ApplyState(std::move(state)); }

} // namespace UI::Modern::PC::World

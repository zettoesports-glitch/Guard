#include "stdafx.h"
#include "UI/Modern/PC/World/RmlStoreLabelLayer.h"

#include "UI/Modern/RmlDocumentHost.h"
#include "UI/Modern/RmlUiDesign.h"

#include <RmlUi/Core/Core.h>
#include <RmlUi/Core/Element.h>
#include <RmlUi/Core/ElementDocument.h>
#include <RmlUi/Core/FileInterface.h>
#include <RmlUi/Core/StreamMemory.h>

#include <array>
#include <cstdio>
#include <sstream>

namespace UI::Modern::PC::World
{

namespace
{
constexpr const char* kDocumentPath =
    "Data/UI/PC/World/store_label.rml";

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
    if (!files) return {};
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

void AppendText(
    Rml::ElementDocument* document,
    Rml::Element* parent,
    const char* className,
    const std::string& text)
{
    if (!document || !parent || text.empty())
        return;

    Rml::ElementPtr element = document->CreateElement("div");
    element->SetClass(className, true);
    element->AppendChild(document->CreateTextNode(text));
    parent->AppendChild(std::move(element));
}

const char* ReputationClass(RmlStoreLabelLayer::Reputation value)
{
    using R = RmlStoreLabelLayer::Reputation;
    switch (value)
    {
    case R::Hero: return "hero";
    case R::Caution: return "caution";
    case R::MurdererOne: return "murderer-one";
    case R::MurdererTwo: return "murderer-two";
    case R::GameMaster: return "game-master";
    case R::Normal:
    default: return "";
    }
}

const char* GuildClass(RmlStoreLabelLayer::GuildRelation value)
{
    using G = RmlStoreLabelLayer::GuildRelation;
    switch (value)
    {
    case G::Friendly: return "guild-friendly";
    case G::Hostile: return "guild-hostile";
    case G::Neutral:
    default: return "";
    }
}
} // namespace

class RmlStoreLabelLayer::Impl
{
public:
    struct Design
    {
        float width = 146.0f;
        float oneTitleHeight = 60.0f;
        float twoTitleHeight = 75.0f;
        float left = -73.0f;
        float top = -80.0f;
        float worldRaise = 60.0f;
        int highlightPeriod = 265;
        float rightMargin = 4.0f;
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
        (void)ParseScalar(values, "StoreLabel-Width", design_.width);
        (void)ParseScalar(values, "StoreLabel-OneTitleHeight", design_.oneTitleHeight);
        (void)ParseScalar(values, "StoreLabel-TwoTitleHeight", design_.twoTitleHeight);
        (void)ParseScalar(values, "StoreLabel-Left", design_.left);
        (void)ParseScalar(values, "StoreLabel-Top", design_.top);
        (void)ParseScalar(values, "StoreLabel-WorldRaise", design_.worldRaise);
        (void)ParseScalar(values, "StoreLabel-HighlightPeriod", design_.highlightPeriod);
        (void)ParseScalar(values, "StoreLabel-RightMargin", design_.rightMargin);

        layer_ = document_->GetElementById("player-name-layer");
        if (!layer_)
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
        if (layer_) layer_->SetProperty("display", "none");
        return host_.Hide();
    }

    void Release()
    {
        layer_ = nullptr;
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
        layer_->SetProperty("display", visible ? "block" : "none");
        if (!visible)
            return true;

        layer_->SetInnerRML("");

        for (std::size_t i = 0; i < state_.labels.size(); ++i)
        {
            const Label& label = state_.labels[i];
            if (!label.visible)
                continue;

            Rml::ElementPtr holder = document_->CreateElement("div");
            holder->SetId(
                Rml::String("player-label-") + std::to_string(i));
            holder->SetClass("player-label", true);
            if (const char* cls = ReputationClass(label.reputation); *cls)
                holder->SetClass(cls, true);
            if (const char* cls = GuildClass(label.guildRelation); *cls)
                holder->SetClass(cls, true);
            holder->SetClass("store-open", label.storeOpen);
            holder->SetClass("highlighted", label.highlighted);
            holder->SetClass("has-guild", !label.guild.empty());
            holder->SetClass("has-chat", !label.chat.empty());

            holder->SetProperty(
                "left", PixelValue(label.screenX));
            holder->SetProperty(
                "top", PixelValue(label.screenY - design_.worldRaise));

            Rml::ElementPtr normal = document_->CreateElement("div");
            normal->SetClass("normal-block", true);

            Rml::ElementPtr stack = document_->CreateElement("div");
            stack->SetClass("name-stack", true);
            AppendText(document_, stack.get(), "name-line player-id", label.playerId);
            AppendText(document_, stack.get(), "name-line guild-line", label.guild);
            AppendText(document_, stack.get(), "name-line title-line", label.title);
            normal->AppendChild(std::move(stack));
            holder->AppendChild(std::move(normal));

            if (label.gensRank > 0)
            {
                Rml::ElementPtr gens = document_->CreateElement("div");
                gens->SetClass("gens-mark", true);
                gens->SetClass(
                    Rml::String("gens-") + std::to_string(label.gensRank),
                    true);
                holder->AppendChild(std::move(gens));
            }

            if (label.castleMark > 0)
            {
                Rml::ElementPtr castle = document_->CreateElement("div");
                castle->SetClass("castle-mark", true);
                castle->SetClass(
                    Rml::String("castle-") +
                        std::to_string(label.castleMark),
                    true);
                holder->AppendChild(std::move(castle));
            }

            if (label.storeOpen)
            {
                Rml::ElementPtr store = document_->CreateElement("div");
                store->SetClass("store-label", true);
                store->SetProperty("left", PixelValue(design_.left));
                store->SetProperty("top", PixelValue(design_.top));
                store->SetProperty("width", PixelValue(design_.width));
                store->SetProperty(
                    "height",
                    PixelValue(
                        label.storeTitle.empty()
                            ? design_.oneTitleHeight
                            : design_.twoTitleHeight));
                AppendText(document_, store.get(), "store-tag", label.storeTag);
                AppendText(document_, store.get(), "store-name", label.storeName);
                AppendText(document_, store.get(), "store-title", label.storeTitle);
                holder->AppendChild(std::move(store));
            }

            if (!label.chat.empty())
            {
                Rml::ElementPtr anchor = document_->CreateElement("div");
                anchor->SetClass("chat-anchor", true);
                Rml::ElementPtr bubble = document_->CreateElement("div");
                bubble->SetClass("chat-bubble", true);
                AppendText(document_, bubble.get(), "chat-text", label.chat);
                Rml::ElementPtr tail = document_->CreateElement("div");
                tail->SetClass("chat-tail", true);
                bubble->AppendChild(std::move(tail));
                anchor->AppendChild(std::move(bubble));
                holder->AppendChild(std::move(anchor));
            }

            layer_->AppendChild(std::move(holder));
        }

        document_->UpdateDocument();
        return true;
    }

private:
    RmlDocumentHost host_;
    Rml::ElementDocument* document_ = nullptr;
    Rml::Element* layer_ = nullptr;
    bool hostVisible_ = false;
    Design design_;
    State state_;
};

RmlStoreLabelLayer::RmlStoreLabelLayer()
    : m_impl(std::make_unique<Impl>()) {}
RmlStoreLabelLayer::~RmlStoreLabelLayer() = default;
RmlStoreLabelLayer::RmlStoreLabelLayer(
    RmlStoreLabelLayer&&) noexcept = default;
RmlStoreLabelLayer& RmlStoreLabelLayer::operator=(
    RmlStoreLabelLayer&&) noexcept = default;

bool RmlStoreLabelLayer::Load(bool show) { return m_impl && m_impl->Load(show); }
bool RmlStoreLabelLayer::Show() { return m_impl && m_impl->Show(); }
bool RmlStoreLabelLayer::Hide() { return m_impl && m_impl->Hide(); }
void RmlStoreLabelLayer::Release() { if (m_impl) m_impl->Release(); }
bool RmlStoreLabelLayer::IsLoaded() const noexcept { return m_impl && m_impl->IsLoaded(); }
bool RmlStoreLabelLayer::ApplyState(const State& state) { return m_impl && m_impl->ApplyState(state); }

} // namespace UI::Modern::PC::World

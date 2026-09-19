#include "stdafx.h"
#include "UI/Modern/PC/HUD/RmlBuffListLayer.h"

#include "UI/Modern/RmlDocumentHost.h"
#include "UI/Modern/RmlMuButton.h"

#include <RmlUi/Core/Element.h>
#include <RmlUi/Core/ElementDocument.h>

#include <algorithm>
#include <array>
#include <cstdio>
#include <memory>
#include <string>
#include <utility>

namespace UI::Modern::PC::HUD
{

namespace
{
constexpr const char* kDocumentPath = "Data/UI/PC/HUD/buff_list.rml";

Rml::String PixelValue(float value)
{
    std::array<char, 64> buffer{};
    std::snprintf(buffer.data(), buffer.size(), "%.3fpx",
                  static_cast<double>(value));
    return Rml::String(buffer.data());
}
} // namespace

class RmlBuffListLayer::Impl
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

        root_ = document_->GetElementById("buff-list");
        buffs_ = document_->GetElementById("_BuffList");
        debuffs_ = document_->GetElementById("_DeBuffList");
        if (!root_ || !buffs_ || !debuffs_)
        {
            Release();
            return false;
        }

        shown_ = show;
        dirty_ = true;
        return Refresh();
    }

    [[nodiscard]] bool Show()
    {
        if (!host_.IsLoaded() && !Load(false))
            return false;
        shown_ = true;
        state_.visible = true;
        dirty_ = true;
        return host_.Show();
    }

    [[nodiscard]] bool Hide()
    {
        shown_ = false;
        state_.visible = false;
        return host_.Hide();
    }

    void Release()
    {
        buttons_.clear();
        root_ = nullptr;
        buffs_ = nullptr;
        debuffs_ = nullptr;
        document_ = nullptr;
        host_.Release();
        clicked_.reset();
        dirty_ = false;
    }

    [[nodiscard]] bool IsLoaded() const noexcept
    {
        return host_.IsLoaded() && document_ != nullptr;
    }

    void SetViewport(int width, int height) noexcept
    {
        state_.viewportWidth = std::max(0, width);
        state_.viewportHeight = std::max(0, height);
        dirty_ = true;
    }

    [[nodiscard]] bool ApplyState(State state)
    {
        const int w = state_.viewportWidth;
        const int h = state_.viewportHeight;
        state_ = std::move(state);
        if (state_.viewportWidth <= 0)
            state_.viewportWidth = w;
        if (state_.viewportHeight <= 0)
            state_.viewportHeight = h;
        dirty_ = true;
        return !IsLoaded() || Refresh();
    }

    [[nodiscard]] bool Update()
    {
        if (!IsLoaded())
            return false;

        bool changed = false;
        for (auto& binding : buttons_)
        {
            changed |= binding.button->Update();
            if (binding.button->ConsumeClicked())
            {
                clicked_ = binding.buffState;
                changed = true;
            }
        }

        if (dirty_)
        {
            changed |= Refresh();
            dirty_ = false;
        }
        return changed;
    }

    [[nodiscard]] std::optional<int> ConsumeClickedBuff()
    {
        return std::exchange(clicked_, std::nullopt);
    }

private:
    struct ButtonBinding
    {
        std::unique_ptr<RmlMuButton> button;
        int buffState = 0;
    };

    [[nodiscard]] bool Refresh()
    {
        if (!IsLoaded())
            return false;

        root_->SetProperty(
            "display", state_.visible ? "block" : "none");

        constexpr float authoredWidth = 366.0f;
        constexpr float authoredHeight = 36.0f;
        constexpr float smallScale = 0.83f;
        constexpr float resizeHeight = 700.0f;

        const float scale =
            state_.viewportHeight > 0 &&
            static_cast<float>(state_.viewportHeight) < resizeHeight
                ? smallScale : 1.0f;

        const float left = std::max(
            0.0f,
            (static_cast<float>(state_.viewportWidth) -
             authoredWidth * scale) * 0.5f);
        root_->SetProperty("left", PixelValue(left));
        root_->SetProperty("top", PixelValue(15.0f));
        root_->SetProperty(
            "transform",
            scale < 1.0f ? "scale(0.830000)" : "scale(1.000000)");

        BuildRows();
        document_->UpdateDocument();
        return true;
    }

    void BuildRows()
    {
        buttons_.clear();
        buffs_->SetInnerRML("");
        debuffs_->SetInnerRML("");

        int buffIndex = 0;
        int debuffIndex = 0;
        for (const Entry& entry : state_.entries)
        {
            Rml::Element* parent = entry.debuff ? debuffs_ : buffs_;
            int& index = entry.debuff ? debuffIndex : buffIndex;

            Rml::ElementPtr icon = document_->CreateElement("div");
            if (!icon)
                continue;

            icon->SetId(
                Rml::String("buff-icon-") +
                std::to_string(entry.buffState));
            icon->SetClass(
                entry.debuff ? "deBuffIcon" : "BuffIcon", true);
            icon->SetClass(
                Rml::String("buff-state-") +
                std::to_string(entry.buffState), true);

            const int column = index % 8;
            const int row = index / 8;
            icon->SetProperty(
                "left", PixelValue(static_cast<float>(column * 34)));
            icon->SetProperty(
                "top", PixelValue(static_cast<float>(row * 36)));

            Rml::ElementPtr image = document_->CreateElement("div");
            image->SetClass("buff-image", true);
            image->AppendChild(document_->CreateTextNode(
                std::to_string(entry.buffState)));
            icon->AppendChild(std::move(image));

            if (entry.referenceCount > 1)
            {
                Rml::ElementPtr count =
                    document_->CreateElement("span");
                count->SetClass("buff-count", true);
                count->AppendChild(document_->CreateTextNode(
                    std::to_string(entry.referenceCount)));
                icon->AppendChild(std::move(count));
            }

            if (!entry.remainingTime.empty())
            {
                Rml::ElementPtr time =
                    document_->CreateElement("span");
                time->SetClass("buff-time", true);
                time->AppendChild(document_->CreateTextNode(
                    entry.remainingTime));
                icon->AppendChild(std::move(time));
            }

            Rml::Element* raw = parent->AppendChild(std::move(icon));
            if (raw)
            {
                auto button = std::make_unique<RmlMuButton>();
                button->Bind(raw);
                buttons_.push_back(
                    {std::move(button), entry.buffState});
            }

            ++index;
        }
    }

    RmlDocumentHost host_;
    Rml::ElementDocument* document_ = nullptr;
    Rml::Element* root_ = nullptr;
    Rml::Element* buffs_ = nullptr;
    Rml::Element* debuffs_ = nullptr;
    std::vector<ButtonBinding> buttons_;

    State state_;
    bool shown_ = false;
    bool dirty_ = false;
    std::optional<int> clicked_;
};

RmlBuffListLayer::RmlBuffListLayer()
    : m_impl(std::make_unique<Impl>()) {}
RmlBuffListLayer::~RmlBuffListLayer() = default;
RmlBuffListLayer::RmlBuffListLayer(RmlBuffListLayer&&) noexcept = default;
RmlBuffListLayer& RmlBuffListLayer::operator=(
    RmlBuffListLayer&&) noexcept = default;

bool RmlBuffListLayer::Load(bool show)
{ return m_impl && m_impl->Load(show); }
bool RmlBuffListLayer::Show()
{ return m_impl && m_impl->Show(); }
bool RmlBuffListLayer::Hide()
{ return m_impl && m_impl->Hide(); }
void RmlBuffListLayer::Release()
{ if (m_impl) m_impl->Release(); }
bool RmlBuffListLayer::IsLoaded() const noexcept
{ return m_impl && m_impl->IsLoaded(); }
void RmlBuffListLayer::SetViewport(int width, int height) noexcept
{ if (m_impl) m_impl->SetViewport(width, height); }
bool RmlBuffListLayer::ApplyState(State state)
{ return m_impl && m_impl->ApplyState(std::move(state)); }
bool RmlBuffListLayer::Update()
{ return m_impl && m_impl->Update(); }
std::optional<int> RmlBuffListLayer::ConsumeClickedBuff()
{ return m_impl ? m_impl->ConsumeClickedBuff() : std::nullopt; }

} // namespace UI::Modern::PC::HUD

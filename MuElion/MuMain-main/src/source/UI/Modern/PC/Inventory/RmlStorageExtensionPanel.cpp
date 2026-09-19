#include "stdafx.h"
#include "UI/Modern/PC/Inventory/RmlStorageExtensionPanel.h"

#include "UI/Modern/RmlDocumentHost.h"
#include "UI/Modern/RmlMuButton.h"
#include "UI/Modern/RmlMuMovablePanel.h"
#include "UI/Modern/RmlMuSlot.h"
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

namespace UI::Modern::PC::Inventory
{

namespace
{
constexpr const char* kDocumentPath =
    "Data/UI/PC/Inventory/storage_extension.rml";

Rml::String PixelValue(float value)
{
    std::array<char, 64> buffer{};
    std::snprintf(buffer.data(), buffer.size(), "%.3fpx",
                  static_cast<double>(value));
    return Rml::String(buffer.data());
}

Rml::String ScaleValue(float value)
{
    std::array<char, 64> buffer{};
    std::snprintf(buffer.data(), buffer.size(), "scale(%.6f)",
                  static_cast<double>(value));
    return Rml::String(buffer.data());
}

bool ParsePair(const RmlUiDesign::Values& values, const char* key,
               float& a, float& b)
{
    const auto it = values.find(key);
    if (it == values.end())
        return false;
    std::istringstream stream(it->second);
    return static_cast<bool>(stream >> a >> b);
}

bool ParseQuad(const RmlUiDesign::Values& values, const char* key,
               float& a, float& b, float& c, float& d)
{
    const auto it = values.find(key);
    if (it == values.end())
        return false;
    std::istringstream stream(it->second);
    return static_cast<bool>(stream >> a >> b >> c >> d);
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
        reinterpret_cast<const Rml::byte*>(contents.data()), contents.size());
    return RmlUiDesign::Parse(&stream);
}

Rml::Element* Element(Rml::ElementDocument* document, const Rml::String& id)
{
    return document ? document->GetElementById(id) : nullptr;
}
} // namespace

class RmlStorageExtensionPanel::Impl
{
public:
    struct Design
    {
        float width = 327.0f;
        float height = 639.0f;
        float gridX = 60.0f;
        float gridY = 102.0f;
        float pitchX = 26.0f;
        float pitchY = 26.0f;
        float initialX = 360.0f;
        float initialY = 40.0f;
    };

    Impl() : host_(kDocumentPath) {}
    ~Impl() { Release(); }

    bool Load(bool show)
    {
        if (host_.IsLoaded())
            return show ? Show() : true;
        if (!host_.Load(show))
            return false;
        document_ = host_.GetDocument();
        if (!document_)
            return false;
        ReadDesign();
        if (!Bind() || !CreateSlots())
        {
            Release();
            return false;
        }
        if (show)
            state_.visible = true;
        return ApplyState(state_);
    }

    bool Show()
    {
        if (!host_.IsLoaded() && !Load(false))
            return false;
        state_.visible = true;
        const bool result = host_.Show();
        if (result)
            (void)ApplyState(state_);
        return result;
    }

    bool Hide()
    {
        state_.visible = false;
        if (root_)
            root_->SetProperty("display", "none");
        return host_.Hide();
    }

    void Release()
    {
        mover_.Unbind();
        close_.Unbind();
        exit_.Unbind();
        for (auto& slot : slots_)
            slot.Unbind();
        slotElements_.fill(nullptr);
        root_ = drag_ = title_ = grid_ = nullptr;
        document_ = nullptr;
        pending_.reset();
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
            state_.visible && state_.viewportWidth > 0 && state_.viewportHeight > 0;
        root_->SetProperty("display", visible ? "block" : "none");
        if (!visible)
            return true;

        ApplyLayout();
        title_->SetInnerRML(state_.title);
        (void)close_.Update();
        (void)exit_.Update();

        for (std::size_t i = 0; i < slots_.size(); ++i)
        {
            const auto& s = state_.slots[i];
            slots_[i].SetVisible(s.visible);
            slots_[i].SetEnabled(s.enabled);
            slots_[i].SetIconFrame(s.iconFrame);
            if (slotElements_[i])
            {
                slotElements_[i]->SetClass("occupied", s.occupied);
                slotElements_[i]->SetClass("selected", s.selected);
            }
            (void)slots_[i].Update();
        }

        document_->UpdateDocument();
        return true;
    }

    bool Update()
    {
        if (!IsLoaded())
            return false;

        bool changed = close_.Update() | exit_.Update();

        if (close_.ConsumeClicked())
        {
            pending_ = Action{ActionType::Close, 0};
            changed = true;
        }
        if (exit_.ConsumeClicked())
        {
            pending_ = Action{ActionType::Exit, 0};
            changed = true;
        }

        for (std::size_t i = 0; i < slots_.size(); ++i)
        {
            changed |= slots_[i].Update();
            if (slots_[i].ConsumeClicked())
            {
                pending_ = Action{ActionType::SlotPrimary, i};
                changed = true;
            }
            if (slots_[i].ConsumeSecondaryClicked())
            {
                pending_ = Action{ActionType::SlotSecondary, i};
                changed = true;
            }
        }

        changed |= mover_.ConsumePositionChanged();
        return changed;
    }

    std::optional<Action> ConsumeAction()
    {
        return std::exchange(pending_, std::nullopt);
    }

private:
    void ReadDesign()
    {
        const auto values = LoadDesignValues();
        (void)ParsePair(values, "Panel-Size", design_.width, design_.height);
        (void)ParseQuad(values, "Panel-Grids",
                        design_.gridX, design_.gridY,
                        design_.pitchX, design_.pitchY);
        (void)ParsePair(values, "Panel-Initial",
                        design_.initialX, design_.initialY);
    }

    bool Bind()
    {
        root_ = Element(document_, "panel");
        drag_ = Element(document_, "btnDrag");
        title_ = Element(document_, "tfTitle");
        grid_ = Element(document_, "storage-grid");
        Rml::Element* close = Element(document_, "btnClose");
        Rml::Element* exit = Element(document_, "btnExit");

        if (!root_ || !drag_ || !title_ || !grid_ || !close || !exit)
            return false;

        close_.Bind(close);
        exit_.Bind(exit);
        mover_.Bind(root_, drag_);
        return true;
    }

    bool CreateSlots()
    {
        grid_->SetInnerRML("");
        for (std::size_t i = 0; i < slots_.size(); ++i)
        {
            auto element = document_->CreateElement("div");
            if (!element)
                return false;
            element->SetId(Rml::String("isSlot") + std::to_string(i));
            element->SetClass("mu-item-slot", true);
            element->SetClass("storage-slot", true);
            Rml::Element* raw = grid_->AppendChild(std::move(element));
            if (!raw)
                return false;
            slotElements_[i] = raw;
            slots_[i].Bind(raw);
        }
        return true;
    }

    void ApplyLayout()
    {
        const float vw = static_cast<float>(state_.viewportWidth);
        const float vh = static_cast<float>(state_.viewportHeight);
        const float scale = std::clamp(
            std::min(vw / std::max(1.0f, design_.initialX + design_.width),
                     vh / std::max(1.0f, design_.initialY + design_.height)),
            0.25f, 1.0f);

        root_->SetProperty("width", PixelValue(design_.width));
        root_->SetProperty("height", PixelValue(design_.height));
        root_->SetProperty("transform-origin", "0 0");
        root_->SetProperty("transform", ScaleValue(scale));

        mover_.SetMetrics(vw / scale, vh / scale,
                          design_.width, design_.height, 0.0f, 0.0f);
        const Rml::Vector2f pos = mover_.GetPosition();
        if (pos.x == 0.0f && pos.y == 0.0f)
            mover_.SetPosition(design_.initialX, design_.initialY);

        grid_->SetProperty("left", PixelValue(design_.gridX));
        grid_->SetProperty("top", PixelValue(design_.gridY));

        for (std::size_t i = 0; i < slotElements_.size(); ++i)
        {
            Rml::Element* e = slotElements_[i];
            const std::size_t column = i % kColumns;
            const std::size_t row = i / kColumns;
            e->SetProperty("left", PixelValue(column * design_.pitchX));
            e->SetProperty("top", PixelValue(row * design_.pitchY));
            e->SetProperty("width", PixelValue(design_.pitchX));
            e->SetProperty("height", PixelValue(design_.pitchY));
        }
    }

    RmlDocumentHost host_;
    Rml::ElementDocument* document_ = nullptr;
    Rml::Element* root_ = nullptr;
    Rml::Element* drag_ = nullptr;
    Rml::Element* title_ = nullptr;
    Rml::Element* grid_ = nullptr;

    std::array<Rml::Element*, kSlotCount> slotElements_{};
    std::array<RmlMuSlot, kSlotCount> slots_{};
    RmlMuButton close_;
    RmlMuButton exit_;
    RmlMuMovablePanel mover_;

    Design design_;
    State state_;
    std::optional<Action> pending_;
};

RmlStorageExtensionPanel::RmlStorageExtensionPanel()
    : m_impl(std::make_unique<Impl>()) {}
RmlStorageExtensionPanel::~RmlStorageExtensionPanel() = default;
RmlStorageExtensionPanel::RmlStorageExtensionPanel(
    RmlStorageExtensionPanel&&) noexcept = default;
RmlStorageExtensionPanel& RmlStorageExtensionPanel::operator=(
    RmlStorageExtensionPanel&&) noexcept = default;

bool RmlStorageExtensionPanel::Load(bool show) { return m_impl && m_impl->Load(show); }
bool RmlStorageExtensionPanel::Show() { return m_impl && m_impl->Show(); }
bool RmlStorageExtensionPanel::Hide() { return m_impl && m_impl->Hide(); }
void RmlStorageExtensionPanel::Release() { if (m_impl) m_impl->Release(); }
bool RmlStorageExtensionPanel::IsLoaded() const noexcept { return m_impl && m_impl->IsLoaded(); }
bool RmlStorageExtensionPanel::ApplyState(const State& state) { return m_impl && m_impl->ApplyState(state); }
bool RmlStorageExtensionPanel::Update() { return m_impl && m_impl->Update(); }
std::optional<RmlStorageExtensionPanel::Action> RmlStorageExtensionPanel::ConsumeAction()
{
    return m_impl ? m_impl->ConsumeAction() : std::nullopt;
}

} // namespace UI::Modern::PC::Inventory

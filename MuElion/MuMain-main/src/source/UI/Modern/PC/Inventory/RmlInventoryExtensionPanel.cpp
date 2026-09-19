#include "stdafx.h"
#include "UI/Modern/PC/Inventory/RmlInventoryExtensionPanel.h"

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
#include <vector>

namespace UI::Modern::PC::Inventory
{

namespace
{
constexpr const char* kDocumentPath =
    "Data/UI/PC/Inventory/inventory_extension.rml";

Rml::String PixelValue(float value)
{
    std::array<char, 64> buffer{};
    std::snprintf(
        buffer.data(), buffer.size(), "%.3fpx",
        static_cast<double>(value));
    return Rml::String(buffer.data());
}

Rml::String ScaleValue(float value)
{
    std::array<char, 64> buffer{};
    std::snprintf(
        buffer.data(), buffer.size(), "scale(%.6f)",
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

bool ParsePair(
    const RmlUiDesign::Values& values,
    const char* key,
    float& a,
    float& b)
{
    const auto it = values.find(key);
    if (it == values.end())
        return false;

    std::istringstream stream(it->second);
    return static_cast<bool>(stream >> a >> b);
}

std::vector<float> ParseList(
    const RmlUiDesign::Values& values,
    const char* key,
    std::initializer_list<float> fallback)
{
    const auto it = values.find(key);
    if (it == values.end())
        return {fallback.begin(), fallback.end()};

    std::istringstream stream(it->second);
    std::vector<float> result;
    float value = 0.0f;
    while (stream >> value)
        result.push_back(value);

    if (result.empty())
        result.assign(fallback.begin(), fallback.end());
    return result;
}

RmlUiDesign::Values LoadDesignValues()
{
    Rml::FileInterface* files = Rml::GetFileInterface();
    if (!files)
        return {};

    Rml::String contents;
    if (!files->LoadFile(kDocumentPath, contents) ||
        contents.empty())
        return {};

    Rml::StreamMemory stream(
        reinterpret_cast<const Rml::byte*>(contents.data()),
        contents.size());
    return RmlUiDesign::Parse(&stream);
}

Rml::Element* RequiredElement(
    Rml::ElementDocument* document,
    const Rml::String& id)
{
    return document ? document->GetElementById(id) : nullptr;
}

} // namespace

class RmlInventoryExtensionPanel::Impl
{
public:
    struct Design
    {
        float width = 327.5f;
        std::array<float, kBagCount> heights{
            223.0f, 361.0f, 499.05f, 636.0f};
        std::array<float, kBagCount> gridX{
            60.0f, 60.0f, 60.0f, 60.0f};
        std::array<float, kBagCount> gridY{
            90.0f, 230.0f, 366.0f, 504.0f};
        float pitchX = 26.0f;
        float pitchY = 26.0f;
        float referenceWidth = 640.0f;
        float referenceHeight = 480.0f;
        float initialX = 21.0f;
        float initialY = 40.0f;
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
        if (!BindElements() || !CreateSlots())
        {
            Release();
            return false;
        }

        if (show)
            state_.visible = true;
        return ApplyState(state_);
    }

    [[nodiscard]] bool Show()
    {
        if (!host_.IsLoaded() && !Load(false))
            return false;

        state_.visible = true;
        const bool shown = host_.Show();
        if (shown)
            (void)ApplyState(state_);
        return shown;
    }

    [[nodiscard]] bool Hide()
    {
        state_.visible = false;
        if (root_)
            root_->SetProperty("display", "none");
        return host_.Hide();
    }

    void Release()
    {
        mover_.Unbind();
        closeButton_.Unbind();

        for (auto& slot : slots_)
            slot.Unbind();

        slotElements_.fill(nullptr);
        bagContainers_.fill(nullptr);
        bagLocked_.fill(nullptr);

        document_ = nullptr;
        root_ = nullptr;
        title_ = nullptr;
        drag_ = nullptr;
        closeElement_ = nullptr;

        pendingAction_.reset();
        host_.Release();
    }

    [[nodiscard]] bool IsLoaded() const noexcept
    {
        return host_.IsLoaded() && document_ != nullptr;
    }

    [[nodiscard]] bool ApplyState(const State& state)
    {
        state_ = state;
        state_.openedBags =
            std::min<std::size_t>(state_.openedBags, kBagCount);

        if (!IsLoaded())
            return false;

        const bool visible =
            state_.visible &&
            state_.viewportWidth > 0 &&
            state_.viewportHeight > 0;
        root_->SetProperty("display", visible ? "block" : "none");
        if (!visible)
            return true;

        ApplyLayout();
        title_->SetInnerRML(state_.title);

        for (std::size_t bag = 0; bag < kBagCount; ++bag)
        {
            const bool opened = bag < state_.openedBags;
            if (bagContainers_[bag])
                bagContainers_[bag]->SetProperty(
                    "display", opened ? "block" : "none");
            if (bagLocked_[bag])
                bagLocked_[bag]->SetProperty(
                    "display", opened ? "none" : "block");
        }

        for (std::size_t i = 0; i < slots_.size(); ++i)
        {
            const std::size_t bag = i / kSlotsPerBag;
            const bool bagOpen = bag < state_.openedBags;
            const SlotState& slotState = state_.slots[i];

            slots_[i].SetVisible(bagOpen && slotState.visible);
            slots_[i].SetEnabled(bagOpen && slotState.enabled);
            slots_[i].SetIconFrame(slotState.iconFrame);

            if (slotElements_[i])
            {
                slotElements_[i]->SetClass(
                    "occupied", slotState.occupied);
                slotElements_[i]->SetClass(
                    "selected", slotState.selected);
            }

            (void)slots_[i].Update();
        }

        (void)closeButton_.Update();
        document_->UpdateDocument();
        return true;
    }

    [[nodiscard]] bool Update()
    {
        if (!IsLoaded())
            return false;

        bool changed = closeButton_.Update();
        if (closeButton_.ConsumeClicked())
        {
            pendingAction_ = Action{ActionType::Close, 0};
            changed = true;
        }

        const std::size_t activeSlots =
            state_.openedBags * kSlotsPerBag;
        for (std::size_t i = 0; i < activeSlots; ++i)
        {
            changed |= slots_[i].Update();

            if (slots_[i].ConsumeClicked())
            {
                pendingAction_ =
                    Action{ActionType::SlotPrimary, i};
                changed = true;
            }

            if (slots_[i].ConsumeSecondaryClicked())
            {
                pendingAction_ =
                    Action{ActionType::SlotSecondary, i};
                changed = true;
            }
        }

        changed |= mover_.ConsumePositionChanged();
        return changed;
    }

    [[nodiscard]] std::optional<Action> ConsumeAction()
    {
        return std::exchange(pendingAction_, std::nullopt);
    }

private:
    void ReadDesign()
    {
        const auto values = LoadDesignValues();

        (void)ParseScalar(
            values, "Extension-Width", design_.width);
        (void)ParseScalar(
            values, "Extension-PitchX", design_.pitchX);
        (void)ParseScalar(
            values, "Extension-PitchY", design_.pitchY);
        (void)ParsePair(
            values, "Extension-Reference",
            design_.referenceWidth, design_.referenceHeight);
        (void)ParsePair(
            values, "Extension-InitialPosition",
            design_.initialX, design_.initialY);

        const auto heights = ParseList(
            values, "Extension-Heights",
            {223.0f, 361.0f, 499.05f, 636.0f});
        const auto xs = ParseList(
            values, "Extension-GridX",
            {60.0f, 60.0f, 60.0f, 60.0f});
        const auto ys = ParseList(
            values, "Extension-GridY",
            {90.0f, 230.0f, 366.0f, 504.0f});

        for (std::size_t i = 0; i < kBagCount; ++i)
        {
            if (i < heights.size())
                design_.heights[i] = heights[i];
            if (i < xs.size())
                design_.gridX[i] = xs[i];
            if (i < ys.size())
                design_.gridY[i] = ys[i];
        }
    }

    [[nodiscard]] bool BindElements()
    {
        root_ = RequiredElement(document_, "extension");
        title_ = RequiredElement(document_, "tfTitle");
        drag_ = RequiredElement(document_, "btnDrag");
        closeElement_ = RequiredElement(document_, "btnClose");

        if (!root_ || !title_ || !drag_ || !closeElement_)
            return false;

        for (std::size_t bag = 0; bag < kBagCount; ++bag)
        {
            bagContainers_[bag] = RequiredElement(
                document_,
                "extension-grid-" + std::to_string(bag));
            bagLocked_[bag] = RequiredElement(
                document_,
                "extension-locked-" + std::to_string(bag));

            if (!bagContainers_[bag] || !bagLocked_[bag])
                return false;
        }

        closeButton_.Bind(closeElement_);
        mover_.Bind(root_, drag_);
        return true;
    }

    [[nodiscard]] bool CreateSlots()
    {
        if (!document_)
            return false;

        for (std::size_t bag = 0; bag < kBagCount; ++bag)
        {
            Rml::Element* container = bagContainers_[bag];
            if (!container)
                return false;

            container->SetInnerRML("");

            for (std::size_t local = 0;
                 local < kSlotsPerBag;
                 ++local)
            {
                const std::size_t index =
                    bag * kSlotsPerBag + local;

                Rml::ElementPtr element =
                    document_->CreateElement("div");
                if (!element)
                    return false;

                element->SetId(
                    Rml::String("isSlot") +
                    std::to_string(index));
                element->SetClass("mu-item-slot", true);
                element->SetClass("extension-slot", true);

                Rml::Element* raw =
                    container->AppendChild(std::move(element));
                if (!raw)
                    return false;

                slotElements_[index] = raw;
                slots_[index].Bind(raw);
            }
        }

        return true;
    }

    void ApplyLayout()
    {
        const std::size_t visibleBags = std::max<std::size_t>(
            1u, state_.openedBags);
        const float height =
            design_.heights[visibleBags - 1u];

        const float viewportWidth =
            static_cast<float>(state_.viewportWidth);
        const float viewportHeight =
            static_cast<float>(state_.viewportHeight);

        const float right =
            std::max(1.0f, design_.initialX + design_.width);
        const float bottom =
            std::max(1.0f, design_.initialY + height);
        const float scale = std::clamp(
            std::min(viewportWidth / right, viewportHeight / bottom),
            0.25f, 1.0f);

        root_->SetProperty("width", PixelValue(design_.width));
        root_->SetProperty("height", PixelValue(height));
        root_->SetProperty("transform-origin", "0 0");
        root_->SetProperty("transform", ScaleValue(scale));

        mover_.SetMetrics(
            viewportWidth / scale,
            viewportHeight / scale,
            design_.width,
            height,
            0.0f,
            0.0f);

        const Rml::Vector2f current = mover_.GetPosition();
        if (current.x == 0.0f && current.y == 0.0f)
            mover_.SetPosition(design_.initialX, design_.initialY);

        for (std::size_t bag = 0; bag < kBagCount; ++bag)
        {
            if (bagContainers_[bag])
            {
                bagContainers_[bag]->SetProperty(
                    "left", PixelValue(design_.gridX[bag]));
                bagContainers_[bag]->SetProperty(
                    "top", PixelValue(design_.gridY[bag]));
            }

            if (bagLocked_[bag])
            {
                bagLocked_[bag]->SetProperty(
                    "left", PixelValue(design_.gridX[bag]));
                bagLocked_[bag]->SetProperty(
                    "top", PixelValue(design_.gridY[bag]));
            }

            for (std::size_t local = 0;
                 local < kSlotsPerBag;
                 ++local)
            {
                const std::size_t index =
                    bag * kSlotsPerBag + local;
                Rml::Element* element = slotElements_[index];
                if (!element)
                    continue;

                const std::size_t column = local % 8u;
                const std::size_t row = local / 8u;
                element->SetProperty(
                    "left",
                    PixelValue(
                        static_cast<float>(column) *
                        design_.pitchX));
                element->SetProperty(
                    "top",
                    PixelValue(
                        static_cast<float>(row) *
                        design_.pitchY));
                element->SetProperty(
                    "width", PixelValue(design_.pitchX));
                element->SetProperty(
                    "height", PixelValue(design_.pitchY));
            }
        }
    }

    RmlDocumentHost host_;
    Rml::ElementDocument* document_ = nullptr;

    Rml::Element* root_ = nullptr;
    Rml::Element* title_ = nullptr;
    Rml::Element* drag_ = nullptr;
    Rml::Element* closeElement_ = nullptr;

    std::array<Rml::Element*, kBagCount> bagContainers_{};
    std::array<Rml::Element*, kBagCount> bagLocked_{};
    std::array<Rml::Element*, kSlotCount> slotElements_{};

    std::array<RmlMuSlot, kSlotCount> slots_{};
    RmlMuButton closeButton_;
    RmlMuMovablePanel mover_;

    Design design_;
    State state_;
    std::optional<Action> pendingAction_;
};

RmlInventoryExtensionPanel::RmlInventoryExtensionPanel()
    : m_impl(std::make_unique<Impl>())
{
}

RmlInventoryExtensionPanel::~RmlInventoryExtensionPanel() = default;
RmlInventoryExtensionPanel::RmlInventoryExtensionPanel(
    RmlInventoryExtensionPanel&&) noexcept = default;
RmlInventoryExtensionPanel&
RmlInventoryExtensionPanel::operator=(
    RmlInventoryExtensionPanel&&) noexcept = default;

bool RmlInventoryExtensionPanel::Load(bool show)
{
    return m_impl && m_impl->Load(show);
}

bool RmlInventoryExtensionPanel::Show()
{
    return m_impl && m_impl->Show();
}

bool RmlInventoryExtensionPanel::Hide()
{
    return m_impl && m_impl->Hide();
}

void RmlInventoryExtensionPanel::Release()
{
    if (m_impl)
        m_impl->Release();
}

bool RmlInventoryExtensionPanel::IsLoaded() const noexcept
{
    return m_impl && m_impl->IsLoaded();
}

bool RmlInventoryExtensionPanel::ApplyState(const State& state)
{
    return m_impl && m_impl->ApplyState(state);
}

bool RmlInventoryExtensionPanel::Update()
{
    return m_impl && m_impl->Update();
}

std::optional<RmlInventoryExtensionPanel::Action>
RmlInventoryExtensionPanel::ConsumeAction()
{
    return m_impl
        ? m_impl->ConsumeAction()
        : std::nullopt;
}

} // namespace UI::Modern::PC::Inventory

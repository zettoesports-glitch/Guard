#include "stdafx.h"
#include "UI/Modern/PC/Inventory/RmlInventoryPanel.h"

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
    "Data/UI/PC/Inventory/inventory.rml";

struct Rect
{
    float x = 0.0f;
    float y = 0.0f;
    float width = 0.0f;
    float height = 0.0f;
};

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

bool ParseRect(
    const RmlUiDesign::Values& values,
    const std::string& key,
    Rect& rect)
{
    const auto it = values.find(key);
    if (it == values.end())
        return false;

    std::istringstream stream(it->second);
    return static_cast<bool>(
        stream >> rect.x >> rect.y >> rect.width >> rect.height);
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

Rml::Element* RequiredElement(
    Rml::ElementDocument* document,
    const Rml::String& id)
{
    return document ? document->GetElementById(id) : nullptr;
}

void SetText(Rml::Element* element, const std::string& text)
{
    if (element)
        element->SetInnerRML(text);
}

} // namespace

class RmlInventoryPanel::Impl
{
public:
    struct Design
    {
        float width = 327.0f;
        float height = 639.0f;
        float referenceWidth = 640.0f;
        float referenceHeight = 480.0f;
        float initialX = 360.0f;
        float initialY = 40.0f;
        float gridX = 60.0f;
        float gridY = 370.0f;
        float pitchX = 26.0f;
        float pitchY = 26.0f;
        std::array<Rect, kEquipmentSlotCount> equipment{};
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
        if (!BindElements() || !CreateInventorySlots())
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
        repairButton_.Unbind();
        privateStoreButton_.Unbind();
        extensionButton_.Unbind();
        socketOptionButton_.Unbind();
        setOptionButton_.Unbind();

        for (auto& slot : inventorySlots_)
            slot.Unbind();
        for (auto& slot : equipmentSlots_)
            slot.Unbind();

        inventorySlotElements_.fill(nullptr);
        equipmentSlotElements_.fill(nullptr);

        document_ = nullptr;
        root_ = nullptr;
        drag_ = nullptr;
        title_ = nullptr;
        zen_ = nullptr;
        grid_ = nullptr;

        closeElement_ = nullptr;
        repairElement_ = nullptr;
        privateStoreElement_ = nullptr;
        extensionElement_ = nullptr;
        socketOptionElement_ = nullptr;
        setOptionElement_ = nullptr;

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

        SetText(title_, state_.title);
        SetText(zen_, state_.zen);

        closeButton_.SetEnabled(true);
        repairButton_.SetEnabled(state_.repairEnabled);
        privateStoreButton_.SetEnabled(state_.privateStoreEnabled);
        extensionButton_.SetEnabled(state_.extensionEnabled);
        socketOptionButton_.SetEnabled(state_.socketOptionEnabled);
        setOptionButton_.SetEnabled(state_.setOptionEnabled);

        repairElement_->SetClass("selected", state_.repairActive);
        privateStoreElement_->SetClass(
            "selected", state_.privateStoreOpen);

        (void)closeButton_.Update();
        (void)repairButton_.Update();
        (void)privateStoreButton_.Update();
        (void)extensionButton_.Update();
        (void)socketOptionButton_.Update();
        (void)setOptionButton_.Update();

        for (std::size_t i = 0; i < inventorySlots_.size(); ++i)
            ApplySlotState(
                inventorySlots_[i],
                inventorySlotElements_[i],
                state_.inventory[i]);

        for (std::size_t i = 0; i < equipmentSlots_.size(); ++i)
            ApplySlotState(
                equipmentSlots_[i],
                equipmentSlotElements_[i],
                state_.equipment[i]);

        document_->UpdateDocument();
        return true;
    }

    [[nodiscard]] bool Update()
    {
        if (!IsLoaded())
            return false;

        bool changed = false;

        for (RmlMuButton* button : AllButtons())
            changed |= button->Update();

        auto emit = [&](RmlMuButton& button, ActionType type) {
            if (button.ConsumeClicked())
            {
                pendingAction_ = Action{type, 0};
                changed = true;
            }
        };

        emit(closeButton_, ActionType::Close);
        emit(repairButton_, ActionType::Repair);
        emit(privateStoreButton_, ActionType::PrivateStore);
        emit(extensionButton_, ActionType::Extension);
        emit(socketOptionButton_, ActionType::SocketOption);
        emit(setOptionButton_, ActionType::SetOption);

        for (std::size_t i = 0; i < inventorySlots_.size(); ++i)
        {
            changed |= inventorySlots_[i].Update();
            if (inventorySlots_[i].ConsumeClicked())
            {
                pendingAction_ = Action{
                    ActionType::InventorySlotPrimary, i};
                changed = true;
            }
            if (inventorySlots_[i].ConsumeSecondaryClicked())
            {
                pendingAction_ = Action{
                    ActionType::InventorySlotSecondary, i};
                changed = true;
            }
        }

        for (std::size_t i = 0; i < equipmentSlots_.size(); ++i)
        {
            changed |= equipmentSlots_[i].Update();
            if (equipmentSlots_[i].ConsumeClicked())
            {
                pendingAction_ = Action{
                    ActionType::EquipmentSlotPrimary, i};
                changed = true;
            }
            if (equipmentSlots_[i].ConsumeSecondaryClicked())
            {
                pendingAction_ = Action{
                    ActionType::EquipmentSlotSecondary, i};
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

        (void)ParsePair(
            values, "Inventory-Size",
            design_.width, design_.height);
        (void)ParsePair(
            values, "Inventory-Reference",
            design_.referenceWidth, design_.referenceHeight);
        (void)ParsePair(
            values, "Inventory-InitialPosition",
            design_.initialX, design_.initialY);

        const auto grid = values.find("Inventory-Grid");
        if (grid != values.end())
        {
            std::istringstream stream(grid->second);
            (void)(stream >>
                design_.gridX >> design_.gridY >>
                design_.pitchX >> design_.pitchY);
        }

        for (std::size_t i = 0; i < design_.equipment.size(); ++i)
        {
            (void)ParseRect(
                values,
                "Inventory-Equipment" + std::to_string(i),
                design_.equipment[i]);
        }
    }

    [[nodiscard]] bool BindElements()
    {
        root_ = RequiredElement(document_, "inventory");
        drag_ = RequiredElement(document_, "btnDrag");
        title_ = RequiredElement(document_, "tfTitle");
        zen_ = RequiredElement(document_, "tfZen");
        grid_ = RequiredElement(document_, "inventory-grid");

        closeElement_ = RequiredElement(document_, "btnClose");
        repairElement_ = RequiredElement(document_, "btnRepair");
        privateStoreElement_ =
            RequiredElement(document_, "btnPrivateStore");
        extensionElement_ =
            RequiredElement(document_, "btnExtensionBag");
        socketOptionElement_ =
            RequiredElement(document_, "btnSocketOption");
        setOptionElement_ =
            RequiredElement(document_, "btnSetOption");

        if (!root_ || !drag_ || !title_ || !zen_ || !grid_ ||
            !closeElement_ || !repairElement_ ||
            !privateStoreElement_ || !extensionElement_ ||
            !socketOptionElement_ || !setOptionElement_)
            return false;

        static constexpr std::array<const char*, kEquipmentSlotCount>
            equipmentIds{
                "slot_weapon_right",
                "slot_weapon_left",
                "slot_helm",
                "slot_armor",
                "slot_pants",
                "slot_gloves",
                "slot_boots",
                "slot_wing",
                "slot_helper",
                "slot_amulet",
                "slot_ring_left",
                "slot_ring_right",
            };

        for (std::size_t i = 0; i < equipmentIds.size(); ++i)
        {
            equipmentSlotElements_[i] =
                RequiredElement(document_, equipmentIds[i]);
            if (!equipmentSlotElements_[i])
                return false;

            equipmentSlots_[i].Bind(equipmentSlotElements_[i]);
        }

        closeButton_.Bind(closeElement_);
        repairButton_.Bind(repairElement_);
        privateStoreButton_.Bind(privateStoreElement_);
        extensionButton_.Bind(extensionElement_);
        socketOptionButton_.Bind(socketOptionElement_);
        setOptionButton_.Bind(setOptionElement_);
        mover_.Bind(root_, drag_);
        return true;
    }

    [[nodiscard]] bool CreateInventorySlots()
    {
        if (!document_ || !grid_)
            return false;

        grid_->SetInnerRML("");

        for (std::size_t i = 0; i < inventorySlots_.size(); ++i)
        {
            Rml::ElementPtr element =
                document_->CreateElement("div");
            if (!element)
                return false;

            element->SetId(
                Rml::String("isSlot") + std::to_string(i));
            element->SetClass("mu-item-slot", true);
            element->SetClass("inventory-slot", true);

            Rml::Element* raw =
                grid_->AppendChild(std::move(element));
            if (!raw)
                return false;

            inventorySlotElements_[i] = raw;
            inventorySlots_[i].Bind(raw);
        }

        return true;
    }

    void ApplyLayout()
    {
        const float viewportWidth =
            static_cast<float>(state_.viewportWidth);
        const float viewportHeight =
            static_cast<float>(state_.viewportHeight);

        const float referenceRight =
            std::max(1.0f, design_.initialX + design_.width);
        const float referenceBottom =
            std::max(1.0f, design_.initialY + design_.height);

        const float scale = std::clamp(
            std::min(
                viewportWidth / referenceRight,
                viewportHeight / referenceBottom),
            0.25f, 1.0f);

        root_->SetProperty("width", PixelValue(design_.width));
        root_->SetProperty("height", PixelValue(design_.height));
        root_->SetProperty("transform-origin", "0 0");
        root_->SetProperty("transform", ScaleValue(scale));

        mover_.SetMetrics(
            viewportWidth / scale,
            viewportHeight / scale,
            design_.width,
            design_.height,
            0.0f,
            0.0f);

        const Rml::Vector2f current = mover_.GetPosition();
        if (current.x == 0.0f && current.y == 0.0f)
            mover_.SetPosition(design_.initialX, design_.initialY);

        grid_->SetProperty("left", PixelValue(design_.gridX));
        grid_->SetProperty("top", PixelValue(design_.gridY));

        for (std::size_t i = 0; i < inventorySlotElements_.size(); ++i)
        {
            Rml::Element* element = inventorySlotElements_[i];
            if (!element)
                continue;

            const std::size_t column = i % 8u;
            const std::size_t row = i / 8u;
            element->SetProperty(
                "left",
                PixelValue(static_cast<float>(column) * design_.pitchX));
            element->SetProperty(
                "top",
                PixelValue(static_cast<float>(row) * design_.pitchY));
            element->SetProperty(
                "width", PixelValue(design_.pitchX));
            element->SetProperty(
                "height", PixelValue(design_.pitchY));
        }

        for (std::size_t i = 0; i < equipmentSlotElements_.size(); ++i)
        {
            Rml::Element* element = equipmentSlotElements_[i];
            if (!element)
                continue;

            const Rect& rect = design_.equipment[i];
            element->SetProperty("left", PixelValue(rect.x));
            element->SetProperty("top", PixelValue(rect.y));
            element->SetProperty("width", PixelValue(rect.width));
            element->SetProperty("height", PixelValue(rect.height));
        }
    }

    static void ApplySlotState(
        RmlMuSlot& slot,
        Rml::Element* element,
        const SlotState& state)
    {
        slot.SetVisible(state.visible);
        slot.SetEnabled(state.enabled);
        slot.SetIconFrame(state.iconFrame);

        if (element)
        {
            element->SetClass("occupied", state.occupied);
            element->SetClass("selected", state.selected);
        }

        (void)slot.Update();
    }

    [[nodiscard]] std::array<RmlMuButton*, 6> AllButtons()
    {
        return {
            &closeButton_,
            &repairButton_,
            &privateStoreButton_,
            &extensionButton_,
            &socketOptionButton_,
            &setOptionButton_,
        };
    }

    RmlDocumentHost host_;
    Rml::ElementDocument* document_ = nullptr;

    Rml::Element* root_ = nullptr;
    Rml::Element* drag_ = nullptr;
    Rml::Element* title_ = nullptr;
    Rml::Element* zen_ = nullptr;
    Rml::Element* grid_ = nullptr;

    Rml::Element* closeElement_ = nullptr;
    Rml::Element* repairElement_ = nullptr;
    Rml::Element* privateStoreElement_ = nullptr;
    Rml::Element* extensionElement_ = nullptr;
    Rml::Element* socketOptionElement_ = nullptr;
    Rml::Element* setOptionElement_ = nullptr;

    std::array<Rml::Element*, kInventorySlotCount>
        inventorySlotElements_{};
    std::array<Rml::Element*, kEquipmentSlotCount>
        equipmentSlotElements_{};

    std::array<RmlMuSlot, kInventorySlotCount> inventorySlots_{};
    std::array<RmlMuSlot, kEquipmentSlotCount> equipmentSlots_{};

    RmlMuButton closeButton_;
    RmlMuButton repairButton_;
    RmlMuButton privateStoreButton_;
    RmlMuButton extensionButton_;
    RmlMuButton socketOptionButton_;
    RmlMuButton setOptionButton_;
    RmlMuMovablePanel mover_;

    Design design_;
    State state_;
    std::optional<Action> pendingAction_;
};

RmlInventoryPanel::RmlInventoryPanel()
    : m_impl(std::make_unique<Impl>())
{
}

RmlInventoryPanel::~RmlInventoryPanel() = default;
RmlInventoryPanel::RmlInventoryPanel(RmlInventoryPanel&&) noexcept = default;
RmlInventoryPanel& RmlInventoryPanel::operator=(
    RmlInventoryPanel&&) noexcept = default;

bool RmlInventoryPanel::Load(bool show)
{
    return m_impl && m_impl->Load(show);
}

bool RmlInventoryPanel::Show()
{
    return m_impl && m_impl->Show();
}

bool RmlInventoryPanel::Hide()
{
    return m_impl && m_impl->Hide();
}

void RmlInventoryPanel::Release()
{
    if (m_impl)
        m_impl->Release();
}

bool RmlInventoryPanel::IsLoaded() const noexcept
{
    return m_impl && m_impl->IsLoaded();
}

bool RmlInventoryPanel::ApplyState(const State& state)
{
    return m_impl && m_impl->ApplyState(state);
}

bool RmlInventoryPanel::Update()
{
    return m_impl && m_impl->Update();
}

std::optional<RmlInventoryPanel::Action>
RmlInventoryPanel::ConsumeAction()
{
    return m_impl
        ? m_impl->ConsumeAction()
        : std::nullopt;
}

} // namespace UI::Modern::PC::Inventory

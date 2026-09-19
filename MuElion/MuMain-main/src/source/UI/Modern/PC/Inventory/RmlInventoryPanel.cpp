#include "stdafx.h"
#include "UI/Modern/PC/Inventory/RmlInventoryPanel.h"

#include "UI/Modern/RmlDocumentHost.h"
#include "UI/Modern/RmlMuButton.h"
#include "UI/Modern/RmlMuMovablePanel.h"
#include "UI/Modern/RmlMuSlot.h"
#include "UI/Modern/RmlUiDesign.h"

#include <RmlUi/Core/Context.h>
#include <RmlUi/Core/Core.h>
#include <RmlUi/Core/Element.h>
#include <RmlUi/Core/ElementDocument.h>
#include <RmlUi/Core/FileInterface.h>
#include <RmlUi/Core/StreamMemory.h>
#include <RmlUi/Core/StringUtilities.h>

#include <algorithm>
#include <array>
#include <memory>
#include <sstream>
#include <utility>
#include <vector>

namespace UI::Modern::PC::Inventory
{

namespace
{
constexpr const char* kInventoryPath =
    "Data/UI/PC/Inventory/inventory.rml";
constexpr const char* kExtensionPath =
    "Data/UI/PC/Inventory/inventory_extension.rml";

struct Rect
{
    float x = 0.0f;
    float y = 0.0f;
    float width = 0.0f;
    float height = 0.0f;
};

RmlUiDesign::Values LoadDesignValues(const char* path)
{
    Rml::FileInterface* files = Rml::GetFileInterface();
    if (!files || !path)
        return {};

    Rml::String contents;
    if (!files->LoadFile(path, contents) || contents.empty())
        return {};

    Rml::StreamMemory stream(
        reinterpret_cast<const Rml::byte*>(contents.data()),
        contents.size());
    return RmlUiDesign::Parse(&stream);
}

template <typename T>
bool ParseScalar(const RmlUiDesign::Values& values,
                 const char* key, T& output)
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

bool ParsePair(const RmlUiDesign::Values& values,
               const char* key, float& x, float& y)
{
    const auto it = values.find(key);
    if (it == values.end())
        return false;
    std::istringstream stream(it->second);
    return static_cast<bool>(stream >> x >> y);
}

bool ParseRect(const RmlUiDesign::Values& values,
               const char* key, Rect& rect)
{
    const auto it = values.find(key);
    if (it == values.end())
        return false;
    std::istringstream stream(it->second);
    return static_cast<bool>(
        stream >> rect.x >> rect.y >> rect.width >> rect.height);
}

Rml::Element* ElementById(
    Rml::ElementDocument* document, const Rml::String& id)
{
    return document ? document->GetElementById(id) : nullptr;
}

void SetEncodedText(Rml::Element* element, const std::string& text)
{
    if (element)
        element->SetInnerRML(Rml::StringUtilities::EncodeRml(text));
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

        float extensionWidth = 327.5f;
        std::array<float, 4> extensionHeights{223.0f, 361.0f, 499.05f, 636.0f};
        float extensionInitialX = 21.0f;
        float extensionInitialY = 40.0f;
    };

    Impl()
        : inventoryHost_(kInventoryPath),
          extensionHost_(kExtensionPath)
    {
    }

    ~Impl()
    {
        Release();
    }

    [[nodiscard]] bool Load(bool show)
    {
        if (inventoryHost_.IsLoaded())
        {
            if (show)
                return Show();
            return true;
        }

        if (!inventoryHost_.Load(show))
            return false;

        inventoryDocument_ = inventoryHost_.GetDocument();
        if (!inventoryDocument_)
            return false;

        ReadDesign();
        if (!BindInventory())
        {
            Release();
            return false;
        }

        inventoryVisible_ = show;
        SetEncodedText(titleElement_, title_);
        SetEncodedText(zenElement_, zenText_);
        ApplyInventoryMetrics();
        return true;
    }

    [[nodiscard]] bool Show()
    {
        if (!inventoryHost_.IsLoaded() && !Load(false))
            return false;
        inventoryVisible_ = true;
        return inventoryHost_.Show();
    }

    [[nodiscard]] bool Hide()
    {
        inventoryVisible_ = false;
        return inventoryHost_.Hide();
    }

    void Release()
    {
        UnbindExtension();
        UnbindInventory();

        extensionDocument_ = nullptr;
        inventoryDocument_ = nullptr;
        extensionHost_.Release();
        inventoryHost_.Release();

        pendingAction_.reset();
        pendingSlot_.reset();
    }

    [[nodiscard]] bool IsLoaded() const noexcept
    {
        return inventoryHost_.IsLoaded() && inventoryDocument_;
    }

    [[nodiscard]] bool ShowExtension(bool show)
    {
        if (!show)
        {
            extensionVisible_ = false;
            return extensionHost_.IsLoaded()
                ? extensionHost_.Hide()
                : true;
        }

        if (expandedBagCount_ == 0)
            return false;

        if (!extensionHost_.IsLoaded())
        {
            if (!extensionHost_.Load(false))
                return false;
            extensionDocument_ = extensionHost_.GetDocument();
            if (!extensionDocument_ || !BindExtension())
            {
                extensionHost_.Release();
                extensionDocument_ = nullptr;
                return false;
            }
            ApplyExtensionMetrics();
            ApplyExtensionBagVisibility();
        }

        extensionVisible_ = true;
        return extensionHost_.Show();
    }

    [[nodiscard]] bool IsExtensionLoaded() const noexcept
    {
        return extensionHost_.IsLoaded() && extensionDocument_;
    }

    void SetTitle(std::string title)
    {
        title_ = std::move(title);
        SetEncodedText(titleElement_, title_);
    }

    void SetZenText(std::string zenText)
    {
        zenText_ = std::move(zenText);
        SetEncodedText(zenElement_, zenText_);
    }

    void SetExpandedBagCount(std::size_t bagCount)
    {
        expandedBagCount_ = std::min<std::size_t>(bagCount, 4);
        if (expandedBagCount_ == 0)
        {
            if (extensionHost_.IsLoaded())
                (void)extensionHost_.Hide();
            extensionVisible_ = false;
        }

        if (extensionDocument_)
        {
            ApplyExtensionMetrics();
            ApplyExtensionBagVisibility();
        }
    }

    [[nodiscard]] std::size_t GetExpandedBagCount() const noexcept
    {
        return expandedBagCount_;
    }

    void SetInventorySlotFrame(std::size_t index, int frame)
    {
        if (index < inventorySlots_.size() && inventorySlots_[index])
            inventorySlots_[index]->SetIconFrame(frame);
    }

    void SetEquipmentSlotFrame(std::size_t index, int frame)
    {
        if (index < equipmentSlots_.size() && equipmentSlots_[index])
            equipmentSlots_[index]->SetIconFrame(frame);
    }

    void SetExtensionSlotFrame(std::size_t index, int frame)
    {
        if (index < extensionSlots_.size() && extensionSlots_[index])
            extensionSlots_[index]->SetIconFrame(frame);
    }

    [[nodiscard]] bool Update()
    {
        bool changed = false;

        if (inventoryDocument_)
        {
            for (RmlMuButton* button : Buttons())
                changed |= button->Update();

            if (closeButton_.ConsumeClicked())
                pendingAction_ = Action::Close;
            else if (socketButton_.ConsumeClicked())
                pendingAction_ = Action::SocketOption;
            else if (setButton_.ConsumeClicked())
                pendingAction_ = Action::SetOption;
            else if (repairButton_.ConsumeClicked())
                pendingAction_ = Action::Repair;
            else if (privateStoreButton_.ConsumeClicked())
                pendingAction_ = Action::PrivateStore;
            else if (extensionButton_.ConsumeClicked())
                pendingAction_ = Action::ExtensionBag;

            changed |= PollSlots(inventorySlots_, SlotArea::Inventory, 0);
            changed |= PollSlots(equipmentSlots_, SlotArea::Equipment, 0);
            changed |= inventoryMover_.ConsumePositionChanged();
        }

        if (extensionDocument_)
        {
            changed |= extensionCloseButton_.Update();
            if (extensionCloseButton_.ConsumeClicked())
                pendingAction_ = Action::CloseExtension;

            changed |= PollSlots(extensionSlots_, SlotArea::Extension, 0);
            changed |= extensionMover_.ConsumePositionChanged();
        }

        return changed || pendingAction_.has_value() || pendingSlot_.has_value();
    }

    [[nodiscard]] std::optional<Action> ConsumeAction()
    {
        return std::exchange(pendingAction_, std::nullopt);
    }

    [[nodiscard]] std::optional<SlotRequest> ConsumeSlotRequest()
    {
        return std::exchange(pendingSlot_, std::nullopt);
    }

private:
    void ReadDesign()
    {
        const auto values = LoadDesignValues(kInventoryPath);
        Rect size{};
        if (ParseRect(values, "Inventory-Size", size))
        {
            design_.width = size.x;
            design_.height = size.y;
        }
        else
        {
            float w = design_.width, h = design_.height;
            if (ParsePair(values, "Inventory-Size", w, h))
            {
                design_.width = w;
                design_.height = h;
            }
        }

        (void)ParsePair(values, "Inventory-Reference",
                        design_.referenceWidth, design_.referenceHeight);
        (void)ParsePair(values, "Inventory-InitialPosition",
                        design_.initialX, design_.initialY);

        const auto extensionValues = LoadDesignValues(kExtensionPath);
        (void)ParseScalar(extensionValues, "Extension-Width",
                          design_.extensionWidth);
        (void)ParsePair(extensionValues, "Extension-InitialPosition",
                        design_.extensionInitialX, design_.extensionInitialY);

        const auto heights = extensionValues.find("Extension-Heights");
        if (heights != extensionValues.end())
        {
            std::istringstream stream(heights->second);
            for (float& value : design_.extensionHeights)
                if (!(stream >> value))
                    break;
        }
    }

    [[nodiscard]] bool BindInventory()
    {
        inventoryRoot_ = ElementById(inventoryDocument_, "inventory");
        dragElement_ = ElementById(inventoryDocument_, "btnDrag");
        titleElement_ = ElementById(inventoryDocument_, "tfTitle");
        zenElement_ = ElementById(inventoryDocument_, "tfZen");

        if (!inventoryRoot_ || !dragElement_ || !titleElement_ || !zenElement_)
            return false;

        auto bindButton = [&](RmlMuButton& button, const char* id) {
            Rml::Element* element = ElementById(inventoryDocument_, id);
            if (!element)
                return false;
            button.Bind(element);
            return true;
        };

        if (!bindButton(closeButton_, "btnClose") ||
            !bindButton(socketButton_, "btnSocketOption") ||
            !bindButton(setButton_, "btnSetOption") ||
            !bindButton(repairButton_, "btnRepair") ||
            !bindButton(privateStoreButton_, "btnPrivateStore") ||
            !bindButton(extensionButton_, "btnExtensionBag"))
            return false;

        inventorySlots_.clear();
        inventorySlots_.reserve(64);
        for (std::size_t index = 0; index < 64; ++index)
        {
            auto slot = std::make_unique<RmlMuSlot>();
            Rml::Element* element = ElementById(
                inventoryDocument_,
                Rml::String("isSlot") + std::to_string(index));
            if (!element)
                return false;
            slot->Bind(element);
            inventorySlots_.push_back(std::move(slot));
        }

        constexpr const char* equipmentIds[] = {
            "slot_weapon_right", "slot_weapon_left", "slot_helm",
            "slot_armor", "slot_pants", "slot_gloves", "slot_boots",
            "slot_wing", "slot_helper", "slot_amulet",
            "slot_ring_left", "slot_ring_right", "slot_pentagram"
        };

        equipmentSlots_.clear();
        equipmentSlots_.reserve(std::size(equipmentIds));
        for (const char* id : equipmentIds)
        {
            auto slot = std::make_unique<RmlMuSlot>();
            Rml::Element* element = ElementById(inventoryDocument_, id);
            if (!element)
                return false;
            slot->Bind(element);
            equipmentSlots_.push_back(std::move(slot));
        }

        inventoryMover_.Bind(inventoryRoot_, dragElement_);
        return true;
    }

    [[nodiscard]] bool BindExtension()
    {
        extensionRoot_ = ElementById(extensionDocument_, "extension");
        extensionDragElement_ = ElementById(extensionDocument_, "btnDrag");
        Rml::Element* close = ElementById(extensionDocument_, "btnClose");
        if (!extensionRoot_ || !extensionDragElement_ || !close)
            return false;

        extensionCloseButton_.Bind(close);
        extensionMover_.Bind(extensionRoot_, extensionDragElement_);

        extensionSlots_.clear();
        extensionSlots_.reserve(128);
        for (std::size_t index = 0; index < 128; ++index)
        {
            auto slot = std::make_unique<RmlMuSlot>();
            Rml::Element* element = ElementById(
                extensionDocument_,
                Rml::String("isSlot") + std::to_string(index));
            if (!element)
                return false;
            slot->Bind(element);
            extensionSlots_.push_back(std::move(slot));
        }
        return true;
    }

    void UnbindInventory()
    {
        inventoryMover_.Unbind();
        for (RmlMuButton* button : Buttons())
            button->Unbind();

        inventorySlots_.clear();
        equipmentSlots_.clear();

        inventoryRoot_ = nullptr;
        dragElement_ = nullptr;
        titleElement_ = nullptr;
        zenElement_ = nullptr;
    }

    void UnbindExtension()
    {
        extensionMover_.Unbind();
        extensionCloseButton_.Unbind();
        extensionSlots_.clear();
        extensionRoot_ = nullptr;
        extensionDragElement_ = nullptr;
    }

    [[nodiscard]] std::array<RmlMuButton*, 6> Buttons()
    {
        return {
            &closeButton_, &socketButton_, &setButton_, &repairButton_,
            &privateStoreButton_, &extensionButton_
        };
    }

    [[nodiscard]] bool PollSlots(
        std::vector<std::unique_ptr<RmlMuSlot>>& slots,
        SlotArea area,
        std::size_t baseIndex)
    {
        bool changed = false;
        for (std::size_t i = 0; i < slots.size(); ++i)
        {
            RmlMuSlot* slot = slots[i].get();
            if (!slot)
                continue;

            changed |= slot->Update();
            if (slot->ConsumeClicked())
            {
                pendingSlot_ = SlotRequest{area, baseIndex + i, false};
                changed = true;
            }
            if (slot->ConsumeSecondaryClicked())
            {
                pendingSlot_ = SlotRequest{area, baseIndex + i, true};
                changed = true;
            }
        }
        return changed;
    }

    void ApplyInventoryMetrics()
    {
        if (!inventoryRoot_ || !inventoryDocument_)
            return;

        const Rml::Vector2i dimensions =
            inventoryDocument_->GetContext()
                ? inventoryDocument_->GetContext()->GetDimensions()
                : Rml::Vector2i{};

        inventoryMover_.SetMetrics(
            static_cast<float>(dimensions.x),
            static_cast<float>(dimensions.y),
            design_.width,
            design_.height,
            0.0f,
            0.0f);
        inventoryMover_.SetPosition(design_.initialX, design_.initialY);
    }

    void ApplyExtensionMetrics()
    {
        if (!extensionRoot_ || !extensionDocument_)
            return;

        const std::size_t visibleBags =
            std::clamp<std::size_t>(expandedBagCount_, 1, 4);
        const float height = design_.extensionHeights[visibleBags - 1];

        extensionRoot_->SetProperty(
            "height", Rml::CreateString("%.3fpx", height));

        const Rml::Vector2i dimensions =
            extensionDocument_->GetContext()
                ? extensionDocument_->GetContext()->GetDimensions()
                : Rml::Vector2i{};

        extensionMover_.SetMetrics(
            static_cast<float>(dimensions.x),
            static_cast<float>(dimensions.y),
            design_.extensionWidth,
            height,
            0.0f,
            0.0f);
        extensionMover_.SetPosition(
            design_.extensionInitialX,
            design_.extensionInitialY);
    }

    void ApplyExtensionBagVisibility()
    {
        const std::size_t visibleSlots =
            std::min<std::size_t>(expandedBagCount_ * 32, 128);

        for (std::size_t index = 0; index < extensionSlots_.size(); ++index)
        {
            if (!extensionSlots_[index])
                continue;
            extensionSlots_[index]->SetVisible(index < visibleSlots);
            (void)extensionSlots_[index]->Update();
        }

        if (extensionRoot_)
        {
            for (std::size_t bag = 0; bag < 4; ++bag)
                extensionRoot_->SetClass(
                    Rml::String("bags-") + std::to_string(bag + 1),
                    expandedBagCount_ == bag + 1);
        }
    }

    RmlDocumentHost inventoryHost_;
    RmlDocumentHost extensionHost_;
    Rml::ElementDocument* inventoryDocument_ = nullptr;
    Rml::ElementDocument* extensionDocument_ = nullptr;

    Rml::Element* inventoryRoot_ = nullptr;
    Rml::Element* dragElement_ = nullptr;
    Rml::Element* titleElement_ = nullptr;
    Rml::Element* zenElement_ = nullptr;
    Rml::Element* extensionRoot_ = nullptr;
    Rml::Element* extensionDragElement_ = nullptr;

    RmlMuMovablePanel inventoryMover_;
    RmlMuMovablePanel extensionMover_;

    RmlMuButton closeButton_;
    RmlMuButton socketButton_;
    RmlMuButton setButton_;
    RmlMuButton repairButton_;
    RmlMuButton privateStoreButton_;
    RmlMuButton extensionButton_;
    RmlMuButton extensionCloseButton_;

    std::vector<std::unique_ptr<RmlMuSlot>> inventorySlots_;
    std::vector<std::unique_ptr<RmlMuSlot>> equipmentSlots_;
    std::vector<std::unique_ptr<RmlMuSlot>> extensionSlots_;

    Design design_;
    std::string title_ = "Inventory";
    std::string zenText_;
    std::size_t expandedBagCount_ = 0;
    bool inventoryVisible_ = false;
    bool extensionVisible_ = false;

    std::optional<Action> pendingAction_;
    std::optional<SlotRequest> pendingSlot_;
};

RmlInventoryPanel::RmlInventoryPanel()
    : m_impl(std::make_unique<Impl>())
{
}

RmlInventoryPanel::~RmlInventoryPanel() = default;
RmlInventoryPanel::RmlInventoryPanel(RmlInventoryPanel&&) noexcept = default;
RmlInventoryPanel& RmlInventoryPanel::operator=(RmlInventoryPanel&&) noexcept = default;

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

bool RmlInventoryPanel::ShowExtension(bool show)
{
    return m_impl && m_impl->ShowExtension(show);
}

bool RmlInventoryPanel::IsExtensionLoaded() const noexcept
{
    return m_impl && m_impl->IsExtensionLoaded();
}

void RmlInventoryPanel::SetTitle(std::string title)
{
    if (m_impl)
        m_impl->SetTitle(std::move(title));
}

void RmlInventoryPanel::SetZenText(std::string zenText)
{
    if (m_impl)
        m_impl->SetZenText(std::move(zenText));
}

void RmlInventoryPanel::SetExpandedBagCount(std::size_t bagCount)
{
    if (m_impl)
        m_impl->SetExpandedBagCount(bagCount);
}

std::size_t RmlInventoryPanel::GetExpandedBagCount() const noexcept
{
    return m_impl ? m_impl->GetExpandedBagCount() : 0;
}

void RmlInventoryPanel::SetInventorySlotFrame(std::size_t index, int frame)
{
    if (m_impl)
        m_impl->SetInventorySlotFrame(index, frame);
}

void RmlInventoryPanel::SetEquipmentSlotFrame(std::size_t index, int frame)
{
    if (m_impl)
        m_impl->SetEquipmentSlotFrame(index, frame);
}

void RmlInventoryPanel::SetExtensionSlotFrame(std::size_t index, int frame)
{
    if (m_impl)
        m_impl->SetExtensionSlotFrame(index, frame);
}

bool RmlInventoryPanel::Update()
{
    return m_impl && m_impl->Update();
}

std::optional<RmlInventoryPanel::Action> RmlInventoryPanel::ConsumeAction()
{
    return m_impl ? m_impl->ConsumeAction() : std::nullopt;
}

std::optional<RmlInventoryPanel::SlotRequest>
RmlInventoryPanel::ConsumeSlotRequest()
{
    return m_impl ? m_impl->ConsumeSlotRequest() : std::nullopt;
}

} // namespace UI::Modern::PC::Inventory

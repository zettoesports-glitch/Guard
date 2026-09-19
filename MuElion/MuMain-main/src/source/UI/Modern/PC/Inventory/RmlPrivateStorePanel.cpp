#include "stdafx.h"
#include "UI/Modern/PC/Inventory/RmlPrivateStorePanel.h"

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
#include <RmlUi/Core/Elements/ElementFormControlInput.h>

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
    "Data/UI/PC/Inventory/private_store.rml";

struct Rect
{
    float x = 0.0f;
    float y = 0.0f;
    float width = 0.0f;
    float height = 0.0f;
};

struct Rgb
{
    float r = 0.0f;
    float g = 0.0f;
    float b = 0.0f;
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

Rml::String CssRgb(const Rgb& color)
{
    const int r = std::clamp(
        static_cast<int>(color.r * 255.0f + 0.5f), 0, 255);
    const int g = std::clamp(
        static_cast<int>(color.g * 255.0f + 0.5f), 0, 255);
    const int b = std::clamp(
        static_cast<int>(color.b * 255.0f + 0.5f), 0, 255);

    std::array<char, 64> buffer{};
    std::snprintf(
        buffer.data(), buffer.size(),
        "rgb(%d,%d,%d)", r, g, b);
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

bool ParseRect(
    const RmlUiDesign::Values& values,
    const char* key,
    Rect& rect)
{
    const auto it = values.find(key);
    if (it == values.end())
        return false;

    std::istringstream stream(it->second);
    return static_cast<bool>(
        stream >> rect.x >> rect.y >>
        rect.width >> rect.height);
}

bool ParseRgb(
    const RmlUiDesign::Values& values,
    const char* key,
    Rgb& color)
{
    const auto it = values.find(key);
    if (it == values.end())
        return false;

    std::istringstream stream(it->second);
    return static_cast<bool>(
        stream >> color.r >> color.g >> color.b);
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

class RmlPrivateStorePanel::Impl
{
public:
    struct Design
    {
        float width = 327.0f;
        float height = 289.0f;
        float initialX = 21.0f;
        float initialY = 40.0f;
        float referenceWidth = 640.0f;
        float referenceHeight = 480.0f;
        Rect sellerGrid{58.5f, 118.5f, 208.0f, 104.0f};
        Rect buyerGrid{58.5f, 118.5f, 208.0f, 104.0f};
        std::size_t columns = 8;
        std::size_t rows = 4;
        Rgb allowed{0.1f, 0.4f, 0.8f};
        Rgb banned{1.0f, 0.0f, 0.0f};
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
        if (!BindElements())
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
        openButton_.Unbind();
        closeShopButton_.Unbind();
        closeButton_.Unbind();

        for (auto& slot : slots_)
            slot.Unbind();

        slotElements_.fill(nullptr);

        document_ = nullptr;
        root_ = nullptr;
        drag_ = nullptr;
        title_ = nullptr;
        nameLabel_ = nullptr;
        buyerName_ = nullptr;
        nameInput_ = nullptr;
        openElement_ = nullptr;
        closeShopElement_ = nullptr;
        closeElement_ = nullptr;
        openLabel_ = nullptr;
        closeShopLabel_ = nullptr;

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

        root_->SetClass(
            "seller", state_.mode == Mode::Seller);
        root_->SetClass(
            "buyer", state_.mode == Mode::Buyer);

        title_->SetInnerRML(state_.title);
        nameLabel_->SetInnerRML("Store");
        buyerName_->SetInnerRML(state_.buyerName);

        if (nameInput_ && nameInput_->GetValue() != state_.storeName)
            nameInput_->SetValue(state_.storeName);

        openLabel_->SetInnerRML(state_.openLabel);
        closeShopLabel_->SetInnerRML(state_.closeShopLabel);

        const bool seller = state_.mode == Mode::Seller;
        openButton_.SetVisible(seller && !state_.shopOpen);
        openButton_.SetEnabled(
            seller && !state_.shopOpen && state_.openEnabled);

        closeShopButton_.SetVisible(seller && state_.shopOpen);
        closeShopButton_.SetEnabled(
            seller && state_.shopOpen && state_.closeShopEnabled);

        (void)openButton_.Update();
        (void)closeShopButton_.Update();
        (void)closeButton_.Update();

        for (std::size_t i = 0; i < slots_.size(); ++i)
        {
            const SlotState& slotState = state_.slots[i];
            slots_[i].SetVisible(slotState.visible);
            slots_[i].SetEnabled(slotState.enabled);
            slots_[i].SetIconFrame(slotState.iconFrame);

            Rml::Element* element = slotElements_[i];
            if (element)
            {
                element->SetClass(
                    "occupied", slotState.occupied);
                element->SetClass(
                    "selected", slotState.selected);

                element->SetClass(
                    "drop-allowed",
                    slotState.dropState == DropState::Allowed);
                element->SetClass(
                    "drop-banned",
                    slotState.dropState == DropState::Banned);

                if (slotState.dropState == DropState::Allowed)
                    element->SetProperty(
                        "border-color", CssRgb(design_.allowed));
                else if (slotState.dropState == DropState::Banned)
                    element->SetProperty(
                        "border-color", CssRgb(design_.banned));
                else
                    element->RemoveProperty("border-color");
            }

            (void)slots_[i].Update();
        }

        document_->UpdateDocument();
        return true;
    }

    [[nodiscard]] bool Update()
    {
        if (!IsLoaded())
            return false;

        bool changed = false;

        changed |= openButton_.Update();
        changed |= closeShopButton_.Update();
        changed |= closeButton_.Update();

        if (openButton_.ConsumeClicked())
        {
            pendingAction_ = Action{
                ActionType::OpenShop,
                0,
                nameInput_ ? nameInput_->GetValue() : std::string{}};
            changed = true;
        }

        if (closeShopButton_.ConsumeClicked())
        {
            pendingAction_ =
                Action{ActionType::CloseShop, 0, {}};
            changed = true;
        }

        if (closeButton_.ConsumeClicked())
        {
            pendingAction_ =
                Action{ActionType::CloseWindow, 0, {}};
            changed = true;
        }

        for (std::size_t i = 0; i < slots_.size(); ++i)
        {
            changed |= slots_[i].Update();

            if (slots_[i].ConsumeClicked())
            {
                pendingAction_ =
                    Action{ActionType::SlotPrimary, i, {}};
                changed = true;
            }

            if (slots_[i].ConsumeSecondaryClicked())
            {
                pendingAction_ =
                    Action{ActionType::SlotSecondary, i, {}};
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
            values, "PrivateStore-Width", design_.width);
        (void)ParseScalar(
            values, "PrivateStore-Height", design_.height);
        (void)ParseScalar(
            values, "PrivateStore-InitialX", design_.initialX);
        (void)ParseScalar(
            values, "PrivateStore-InitialY", design_.initialY);
        (void)ParseScalar(
            values, "PrivateStore-ReferenceWidth",
            design_.referenceWidth);
        (void)ParseScalar(
            values, "PrivateStore-ReferenceHeight",
            design_.referenceHeight);
        (void)ParseScalar(
            values, "PrivateStore-GridColumns",
            design_.columns);
        (void)ParseScalar(
            values, "PrivateStore-GridRows",
            design_.rows);
        (void)ParseRect(
            values, "PrivateStore-seller-GridRect",
            design_.sellerGrid);
        (void)ParseRect(
            values, "PrivateStore-buyer-GridRect",
            design_.buyerGrid);
        (void)ParseRgb(
            values, "PrivateStore-DropAllowedColor",
            design_.allowed);
        (void)ParseRgb(
            values, "PrivateStore-DropBannedColor",
            design_.banned);
    }

    [[nodiscard]] bool BindElements()
    {
        root_ = RequiredElement(document_, "private-store");
        drag_ = RequiredElement(document_, "private-store-drag");
        title_ = RequiredElement(document_, "private-store-title");
        nameLabel_ =
            RequiredElement(document_, "private-store-name-label");
        buyerName_ =
            RequiredElement(document_, "private-store-buyer-name");
        nameInput_ = dynamic_cast<Rml::ElementFormControlInput*>(
            RequiredElement(document_, "private-store-name"));
        openElement_ =
            RequiredElement(document_, "private-store-open");
        closeShopElement_ =
            RequiredElement(document_, "private-store-close-shop");
        closeElement_ =
            RequiredElement(document_, "private-store-close");
        openLabel_ =
            RequiredElement(document_, "private-store-open-label");
        closeShopLabel_ =
            RequiredElement(document_, "private-store-close-shop-label");

        if (!root_ || !drag_ || !title_ || !nameLabel_ ||
            !buyerName_ || !nameInput_ || !openElement_ ||
            !closeShopElement_ || !closeElement_ ||
            !openLabel_ || !closeShopLabel_)
            return false;

        nameInput_->SetAttribute("maxlength", 25);

        for (std::size_t i = 0; i < slots_.size(); ++i)
        {
            slotElements_[i] = RequiredElement(
                document_,
                "private-store-slot-" + std::to_string(i));
            if (!slotElements_[i])
                return false;

            slots_[i].Bind(slotElements_[i]);
        }

        openButton_.Bind(openElement_);
        closeShopButton_.Bind(closeShopElement_);
        closeButton_.Bind(closeElement_);
        mover_.Bind(root_, drag_);
        return true;
    }

    void ApplyLayout()
    {
        const float viewportWidth =
            static_cast<float>(state_.viewportWidth);
        const float viewportHeight =
            static_cast<float>(state_.viewportHeight);

        const float right =
            std::max(1.0f, design_.initialX + design_.width);
        const float bottom =
            std::max(1.0f, design_.initialY + design_.height);

        const float scale = std::clamp(
            std::min(viewportWidth / right, viewportHeight / bottom),
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

        const Rect& grid =
            state_.mode == Mode::Seller
                ? design_.sellerGrid
                : design_.buyerGrid;

        const std::size_t columns =
            std::max<std::size_t>(1u, design_.columns);
        const std::size_t rows =
            std::max<std::size_t>(1u, design_.rows);
        const float slotWidth =
            grid.width / static_cast<float>(columns);
        const float slotHeight =
            grid.height / static_cast<float>(rows);

        for (std::size_t i = 0; i < slotElements_.size(); ++i)
        {
            Rml::Element* element = slotElements_[i];
            if (!element)
                continue;

            const std::size_t column = i % columns;
            const std::size_t row = i / columns;

            element->SetProperty(
                "left",
                PixelValue(
                    grid.x +
                    static_cast<float>(column) * slotWidth));
            element->SetProperty(
                "top",
                PixelValue(
                    grid.y +
                    static_cast<float>(row) * slotHeight));
            element->SetProperty(
                "width", PixelValue(slotWidth));
            element->SetProperty(
                "height", PixelValue(slotHeight));
        }
    }

    RmlDocumentHost host_;
    Rml::ElementDocument* document_ = nullptr;

    Rml::Element* root_ = nullptr;
    Rml::Element* drag_ = nullptr;
    Rml::Element* title_ = nullptr;
    Rml::Element* nameLabel_ = nullptr;
    Rml::Element* buyerName_ = nullptr;
    Rml::ElementFormControlInput* nameInput_ = nullptr;

    Rml::Element* openElement_ = nullptr;
    Rml::Element* closeShopElement_ = nullptr;
    Rml::Element* closeElement_ = nullptr;
    Rml::Element* openLabel_ = nullptr;
    Rml::Element* closeShopLabel_ = nullptr;

    std::array<Rml::Element*, kSlotCount> slotElements_{};
    std::array<RmlMuSlot, kSlotCount> slots_{};

    RmlMuButton openButton_;
    RmlMuButton closeShopButton_;
    RmlMuButton closeButton_;
    RmlMuMovablePanel mover_;

    Design design_;
    State state_;
    std::optional<Action> pendingAction_;
};

RmlPrivateStorePanel::RmlPrivateStorePanel()
    : m_impl(std::make_unique<Impl>())
{
}

RmlPrivateStorePanel::~RmlPrivateStorePanel() = default;
RmlPrivateStorePanel::RmlPrivateStorePanel(
    RmlPrivateStorePanel&&) noexcept = default;
RmlPrivateStorePanel& RmlPrivateStorePanel::operator=(
    RmlPrivateStorePanel&&) noexcept = default;

bool RmlPrivateStorePanel::Load(bool show)
{
    return m_impl && m_impl->Load(show);
}

bool RmlPrivateStorePanel::Show()
{
    return m_impl && m_impl->Show();
}

bool RmlPrivateStorePanel::Hide()
{
    return m_impl && m_impl->Hide();
}

void RmlPrivateStorePanel::Release()
{
    if (m_impl)
        m_impl->Release();
}

bool RmlPrivateStorePanel::IsLoaded() const noexcept
{
    return m_impl && m_impl->IsLoaded();
}

bool RmlPrivateStorePanel::ApplyState(const State& state)
{
    return m_impl && m_impl->ApplyState(state);
}

bool RmlPrivateStorePanel::Update()
{
    return m_impl && m_impl->Update();
}

std::optional<RmlPrivateStorePanel::Action>
RmlPrivateStorePanel::ConsumeAction()
{
    return m_impl
        ? m_impl->ConsumeAction()
        : std::nullopt;
}

} // namespace UI::Modern::PC::Inventory

#include "stdafx.h"
#include "UI/Modern/PC/Inventory/RmlPrivateStorePanel.h"

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
#include <RmlUi/Core/Elements/ElementFormControlInput.h>

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
constexpr const char* kDocumentPath =
    "Data/UI/PC/Inventory/private_store.rml";

struct Rect
{
    float x = 0.0f;
    float y = 0.0f;
    float width = 0.0f;
    float height = 0.0f;
};

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

template <typename T>
bool ParseScalar(const RmlUiDesign::Values& values,
                 const char* key, T& output)
{
    const auto it = values.find(key);
    if (it == values.end())
        return false;
    std::istringstream stream(it->second);
    T value{};
    if (!(stream >> value))
        return false;
    output = value;
    return true;
}

bool ParseRect(const RmlUiDesign::Values& values,
               const char* key, Rect& output)
{
    const auto it = values.find(key);
    if (it == values.end())
        return false;
    std::istringstream stream(it->second);
    return static_cast<bool>(
        stream >> output.x >> output.y >> output.width >> output.height);
}

Rml::Element* ElementById(
    Rml::ElementDocument* document, const char* id)
{
    return document ? document->GetElementById(id) : nullptr;
}

void SetEncodedText(Rml::Element* element, const std::string& text)
{
    if (element)
        element->SetInnerRML(Rml::StringUtilities::EncodeRml(text));
}

} // namespace

class RmlPrivateStorePanel::Impl
{
public:
    struct Design
    {
        float width = 327.0f;
        float height = 289.0f;
        int columns = 8;
        int rows = 4;
        Rect sellerGrid{58.5f, 118.5f, 208.0f, 104.0f};
        Rect buyerGrid{58.5f, 118.5f, 208.0f, 104.0f};
        float initialX = 21.0f;
        float initialY = 40.0f;
        float referenceWidth = 640.0f;
        float referenceHeight = 480.0f;
        std::array<float, 3> allowedColor{0.1f, 0.4f, 0.8f};
        std::array<float, 3> bannedColor{1.0f, 0.0f, 0.0f};
    };

    Impl() : host_(kDocumentPath) {}

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
        if (!Bind())
        {
            Release();
            return false;
        }

        visible_ = show;
        ApplyMode();
        ApplyMetrics();
        SetEncodedText(titleElement_, title_);
        SetEncodedText(buyerNameElement_, buyerName_);
        if (storeNameInput_)
            storeNameInput_->SetValue(storeName_);
        return true;
    }

    [[nodiscard]] bool Show()
    {
        if (!host_.IsLoaded() && !Load(false))
            return false;
        visible_ = true;
        return host_.Show();
    }

    [[nodiscard]] bool Hide()
    {
        visible_ = false;
        return host_.Hide();
    }

    void Release()
    {
        mover_.Unbind();
        openButton_.Unbind();
        closeShopButton_.Unbind();
        closeButton_.Unbind();
        slots_.clear();

        root_ = nullptr;
        dragElement_ = nullptr;
        titleElement_ = nullptr;
        buyerNameElement_ = nullptr;
        storeNameInput_ = nullptr;
        document_ = nullptr;

        host_.Release();
        pendingAction_.reset();
        pendingSlot_.reset();
    }

    [[nodiscard]] bool IsLoaded() const noexcept
    {
        return host_.IsLoaded() && document_;
    }

    void SetMode(Mode mode)
    {
        if (mode_ == mode)
            return;
        mode_ = mode;
        ApplyMode();
    }

    [[nodiscard]] Mode GetMode() const noexcept
    {
        return mode_;
    }

    void SetStoreName(std::string name)
    {
        storeName_ = std::move(name);
        if (storeNameInput_)
            storeNameInput_->SetValue(storeName_);
    }

    [[nodiscard]] std::string GetStoreName() const
    {
        if (storeNameInput_)
            return storeNameInput_->GetValue();
        return storeName_;
    }

    void SetBuyerName(std::string name)
    {
        buyerName_ = std::move(name);
        SetEncodedText(buyerNameElement_, buyerName_);
    }

    void SetTitle(std::string title)
    {
        title_ = std::move(title);
        SetEncodedText(titleElement_, title_);
    }

    void SetOpenEnabled(bool enabled)
    {
        openButton_.SetEnabled(enabled);
    }

    void SetCloseShopEnabled(bool enabled)
    {
        closeShopButton_.SetEnabled(enabled);
    }

    void SetSlotFrame(std::size_t index, int frame)
    {
        if (index < slots_.size() && slots_[index])
            slots_[index]->SetIconFrame(frame);
    }

    void SetSlotDropState(std::size_t index, DropState state)
    {
        if (index >= slotElements_.size() || !slotElements_[index])
            return;

        Rml::Element* element = slotElements_[index];
        element->SetClass("drop-allowed", state == DropState::Allowed);
        element->SetClass("drop-banned", state == DropState::Banned);
        element->SetClass("drop-neutral", state == DropState::Neutral);
    }

    [[nodiscard]] bool Update()
    {
        if (!document_)
            return false;

        bool changed = false;
        changed |= openButton_.Update();
        changed |= closeShopButton_.Update();
        changed |= closeButton_.Update();

        if (openButton_.ConsumeClicked())
        {
            if (storeNameInput_)
                storeName_ = storeNameInput_->GetValue();
            pendingAction_ = Action::OpenStore;
            changed = true;
        }
        if (closeShopButton_.ConsumeClicked())
        {
            pendingAction_ = Action::CloseShop;
            changed = true;
        }
        if (closeButton_.ConsumeClicked())
        {
            pendingAction_ = Action::ClosePanel;
            changed = true;
        }

        for (std::size_t index = 0; index < slots_.size(); ++index)
        {
            RmlMuSlot* slot = slots_[index].get();
            if (!slot)
                continue;

            changed |= slot->Update();
            if (slot->ConsumeClicked())
            {
                pendingSlot_ = SlotRequest{index, false};
                changed = true;
            }
            if (slot->ConsumeSecondaryClicked())
            {
                pendingSlot_ = SlotRequest{index, true};
                changed = true;
            }
        }

        changed |= mover_.ConsumePositionChanged();
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
        const auto values = LoadDesignValues();
        (void)ParseScalar(values, "PrivateStore-Width", design_.width);
        (void)ParseScalar(values, "PrivateStore-Height", design_.height);
        (void)ParseScalar(values, "PrivateStore-GridColumns", design_.columns);
        (void)ParseScalar(values, "PrivateStore-GridRows", design_.rows);
        (void)ParseScalar(values, "PrivateStore-InitialX", design_.initialX);
        (void)ParseScalar(values, "PrivateStore-InitialY", design_.initialY);
        (void)ParseScalar(values, "PrivateStore-ReferenceWidth", design_.referenceWidth);
        (void)ParseScalar(values, "PrivateStore-ReferenceHeight", design_.referenceHeight);
        (void)ParseRect(values, "PrivateStore-seller-GridRect", design_.sellerGrid);
        (void)ParseRect(values, "PrivateStore-buyer-GridRect", design_.buyerGrid);

        const auto parseColor = [&](const char* key, std::array<float, 3>& color) {
            const auto it = values.find(key);
            if (it == values.end())
                return;
            std::istringstream stream(it->second);
            (void)(stream >> color[0] >> color[1] >> color[2]);
        };
        parseColor("PrivateStore-DropAllowedColor", design_.allowedColor);
        parseColor("PrivateStore-DropBannedColor", design_.bannedColor);
    }

    [[nodiscard]] bool Bind()
    {
        root_ = ElementById(document_, "private-store");
        dragElement_ = ElementById(document_, "private-store-drag");
        titleElement_ = ElementById(document_, "private-store-title");
        buyerNameElement_ = ElementById(document_, "private-store-buyer-name");
        storeNameInput_ = dynamic_cast<Rml::ElementFormControlInput*>(
            ElementById(document_, "private-store-name"));

        Rml::Element* open = ElementById(document_, "private-store-open");
        Rml::Element* closeShop = ElementById(document_, "private-store-close-shop");
        Rml::Element* close = ElementById(document_, "private-store-close");

        if (!root_ || !dragElement_ || !titleElement_ || !buyerNameElement_ ||
            !storeNameInput_ || !open || !closeShop || !close)
            return false;

        storeNameInput_->SetAttribute("maxlength", 25);

        openButton_.Bind(open);
        closeShopButton_.Bind(closeShop);
        closeButton_.Bind(close);
        mover_.Bind(root_, dragElement_);

        slots_.clear();
        slotElements_.clear();
        slots_.reserve(32);
        slotElements_.reserve(32);
        for (std::size_t index = 0; index < 32; ++index)
        {
            Rml::Element* element = ElementById(
                document_,
                (Rml::String("private-store-slot-") + std::to_string(index)).c_str());
            if (!element)
                return false;

            auto slot = std::make_unique<RmlMuSlot>();
            slot->Bind(element);
            slots_.push_back(std::move(slot));
            slotElements_.push_back(element);
        }

        return true;
    }

    void ApplyMode()
    {
        if (!root_)
            return;

        const bool seller = mode_ == Mode::Seller;
        root_->SetClass("seller", seller);
        root_->SetClass("buyer", !seller);

        openButton_.SetVisible(seller);
        closeShopButton_.SetVisible(seller);
        if (storeNameInput_)
            storeNameInput_->SetProperty("display", seller ? "block" : "none");
        if (buyerNameElement_)
            buyerNameElement_->SetProperty("display", seller ? "none" : "block");

        if (Rml::Element* openLabel =
                ElementById(document_, "private-store-open-label"))
            SetEncodedText(openLabel, seller ? "Open" : "");
        if (Rml::Element* closeLabel =
                ElementById(document_, "private-store-close-shop-label"))
            SetEncodedText(closeLabel, seller ? "Close Shop" : "");

        (void)openButton_.Update();
        (void)closeShopButton_.Update();
    }

    void ApplyMetrics()
    {
        if (!root_ || !document_)
            return;

        const Rml::Vector2i dimensions =
            document_->GetContext()
                ? document_->GetContext()->GetDimensions()
                : Rml::Vector2i{};

        mover_.SetMetrics(
            static_cast<float>(dimensions.x),
            static_cast<float>(dimensions.y),
            design_.width,
            design_.height,
            0.0f,
            0.0f);
        mover_.SetPosition(design_.initialX, design_.initialY);
    }

    RmlDocumentHost host_;
    Rml::ElementDocument* document_ = nullptr;

    Rml::Element* root_ = nullptr;
    Rml::Element* dragElement_ = nullptr;
    Rml::Element* titleElement_ = nullptr;
    Rml::Element* buyerNameElement_ = nullptr;
    Rml::ElementFormControlInput* storeNameInput_ = nullptr;

    RmlMuMovablePanel mover_;
    RmlMuButton openButton_;
    RmlMuButton closeShopButton_;
    RmlMuButton closeButton_;
    std::vector<std::unique_ptr<RmlMuSlot>> slots_;
    std::vector<Rml::Element*> slotElements_;

    Design design_;
    Mode mode_ = Mode::Seller;
    std::string storeName_;
    std::string buyerName_;
    std::string title_ = "Private Store";
    bool visible_ = false;

    std::optional<Action> pendingAction_;
    std::optional<SlotRequest> pendingSlot_;
};

RmlPrivateStorePanel::RmlPrivateStorePanel()
    : m_impl(std::make_unique<Impl>())
{
}

RmlPrivateStorePanel::~RmlPrivateStorePanel() = default;
RmlPrivateStorePanel::RmlPrivateStorePanel(RmlPrivateStorePanel&&) noexcept = default;
RmlPrivateStorePanel& RmlPrivateStorePanel::operator=(RmlPrivateStorePanel&&) noexcept = default;

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

void RmlPrivateStorePanel::SetMode(Mode mode)
{
    if (m_impl)
        m_impl->SetMode(mode);
}

RmlPrivateStorePanel::Mode RmlPrivateStorePanel::GetMode() const noexcept
{
    return m_impl ? m_impl->GetMode() : Mode::Seller;
}

void RmlPrivateStorePanel::SetStoreName(std::string name)
{
    if (m_impl)
        m_impl->SetStoreName(std::move(name));
}

std::string RmlPrivateStorePanel::GetStoreName() const
{
    return m_impl ? m_impl->GetStoreName() : std::string{};
}

void RmlPrivateStorePanel::SetBuyerName(std::string name)
{
    if (m_impl)
        m_impl->SetBuyerName(std::move(name));
}

void RmlPrivateStorePanel::SetTitle(std::string title)
{
    if (m_impl)
        m_impl->SetTitle(std::move(title));
}

void RmlPrivateStorePanel::SetOpenEnabled(bool enabled)
{
    if (m_impl)
        m_impl->SetOpenEnabled(enabled);
}

void RmlPrivateStorePanel::SetCloseShopEnabled(bool enabled)
{
    if (m_impl)
        m_impl->SetCloseShopEnabled(enabled);
}

void RmlPrivateStorePanel::SetSlotFrame(std::size_t index, int frame)
{
    if (m_impl)
        m_impl->SetSlotFrame(index, frame);
}

void RmlPrivateStorePanel::SetSlotDropState(
    std::size_t index, DropState state)
{
    if (m_impl)
        m_impl->SetSlotDropState(index, state);
}

bool RmlPrivateStorePanel::Update()
{
    return m_impl && m_impl->Update();
}

std::optional<RmlPrivateStorePanel::Action>
RmlPrivateStorePanel::ConsumeAction()
{
    return m_impl ? m_impl->ConsumeAction() : std::nullopt;
}

std::optional<RmlPrivateStorePanel::SlotRequest>
RmlPrivateStorePanel::ConsumeSlotRequest()
{
    return m_impl ? m_impl->ConsumeSlotRequest() : std::nullopt;
}

} // namespace UI::Modern::PC::Inventory

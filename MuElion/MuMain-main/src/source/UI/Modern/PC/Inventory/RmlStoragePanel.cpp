#include "stdafx.h"
#include "UI/Modern/PC/Inventory/RmlStoragePanel.h"

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
    "Data/UI/PC/Inventory/storage.rml";

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

bool ParseQuad(
    const RmlUiDesign::Values& values,
    const char* key,
    float& a,
    float& b,
    float& c,
    float& d)
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

void SetText(Rml::Element* element, const std::string& text)
{
    if (element)
        element->SetInnerRML(text);
}

} // namespace

class RmlStoragePanel::Impl
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
        float referenceWidth = 640.0f;
        float referenceHeight = 480.0f;
        float initialX = 21.0f;
        float initialY = 40.0f;
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

        for (RmlMuButton* button : Buttons())
            button->Unbind();

        for (auto& slot : slots_)
            slot.Unbind();

        slotElements_.fill(nullptr);
        document_ = nullptr;
        root_ = nullptr;
        drag_ = nullptr;
        title_ = nullptr;
        grid_ = nullptr;
        chargeGroup_ = nullptr;
        periodGroup_ = nullptr;
        storedZen_ = nullptr;
        charge_ = nullptr;
        remainingTime_ = nullptr;
        lockElement_ = nullptr;

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
        SetText(storedZen_, state_.storedZen);
        SetText(charge_, state_.charge);
        SetText(remainingTime_, state_.remainingTime);

        chargeGroup_->SetProperty(
            "display", state_.chargeVisible ? "block" : "none");
        periodGroup_->SetProperty(
            "display", state_.periodVisible ? "block" : "none");

        lockElement_->SetClass("locked", state_.locked);
        lockElement_->SetClass("unlocked", !state_.locked);
        SetText(lockLabel_, state_.locked ? "Unlock" : "Lock");

        extendButton_.SetEnabled(state_.extendEnabled);

        for (RmlMuButton* button : Buttons())
            (void)button->Update();

        for (std::size_t i = 0; i < slots_.size(); ++i)
        {
            const SlotState& slotState = state_.slots[i];
            slots_[i].SetVisible(slotState.visible);
            slots_[i].SetEnabled(slotState.enabled);
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

        document_->UpdateDocument();
        return true;
    }

    [[nodiscard]] bool Update()
    {
        if (!IsLoaded())
            return false;

        bool changed = false;
        for (RmlMuButton* button : Buttons())
            changed |= button->Update();

        auto consume = [&](RmlMuButton& button, ActionType type) {
            if (button.ConsumeClicked())
            {
                pendingAction_ = Action{type, 0};
                changed = true;
            }
        };

        consume(closeButton_, ActionType::Close);
        consume(exitButton_, ActionType::Exit);
        consume(depositButton_, ActionType::DepositZen);
        consume(withdrawButton_, ActionType::WithdrawZen);
        consume(lockButton_, ActionType::ToggleLock);
        consume(extendButton_, ActionType::Extend);
        consume(zenInfoButton_, ActionType::ZenInfo);

        for (std::size_t i = 0; i < slots_.size(); ++i)
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
        (void)ParsePair(
            values, "Panel-Size",
            design_.width, design_.height);
        (void)ParseQuad(
            values, "Panel-Grids",
            design_.gridX, design_.gridY,
            design_.pitchX, design_.pitchY);
        (void)ParsePair(
            values, "Panel-Reference",
            design_.referenceWidth, design_.referenceHeight);
        (void)ParsePair(
            values, "Panel-Initial",
            design_.initialX, design_.initialY);
    }

    [[nodiscard]] bool BindElements()
    {
        root_ = RequiredElement(document_, "panel");
        drag_ = RequiredElement(document_, "btnDrag");
        title_ = RequiredElement(document_, "tfTitle");
        grid_ = RequiredElement(document_, "storage-grid");
        chargeGroup_ = RequiredElement(document_, "mcCharge");
        periodGroup_ = RequiredElement(document_, "mcPeriodStorage");
        storedZen_ = RequiredElement(document_, "tfKeepZenValue");
        charge_ = RequiredElement(document_, "tfChargeValue");
        remainingTime_ = RequiredElement(document_, "tfRemainTime");

        Rml::Element* close = RequiredElement(document_, "btnClose");
        Rml::Element* exit = RequiredElement(document_, "btnExit");
        Rml::Element* deposit = RequiredElement(document_, "btnDeposit");
        Rml::Element* withdraw = RequiredElement(document_, "btnWithDraw");
        lockElement_ = RequiredElement(document_, "btnLift");
        Rml::Element* extend = RequiredElement(document_, "btnExtend");
        Rml::Element* zenInfo = RequiredElement(document_, "btnZenInfo");
        lockLabel_ = RequiredElement(document_, "btnLift-label");

        if (!root_ || !drag_ || !title_ || !grid_ ||
            !chargeGroup_ || !periodGroup_ || !storedZen_ ||
            !charge_ || !remainingTime_ || !close || !exit ||
            !deposit || !withdraw || !lockElement_ || !extend ||
            !zenInfo || !lockLabel_)
            return false;

        closeButton_.Bind(close);
        exitButton_.Bind(exit);
        depositButton_.Bind(deposit);
        withdrawButton_.Bind(withdraw);
        lockButton_.Bind(lockElement_);
        extendButton_.Bind(extend);
        zenInfoButton_.Bind(zenInfo);
        mover_.Bind(root_, drag_);
        return true;
    }

    [[nodiscard]] bool CreateSlots()
    {
        if (!document_ || !grid_)
            return false;

        grid_->SetInnerRML("");

        for (std::size_t i = 0; i < slots_.size(); ++i)
        {
            Rml::ElementPtr element =
                document_->CreateElement("div");
            if (!element)
                return false;

            element->SetId(
                Rml::String("isSlot") + std::to_string(i));
            element->SetClass("mu-item-slot", true);
            element->SetClass("storage-slot", true);

            Rml::Element* raw =
                grid_->AppendChild(std::move(element));
            if (!raw)
                return false;

            slotElements_[i] = raw;
            slots_[i].Bind(raw);
        }

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

        grid_->SetProperty("left", PixelValue(design_.gridX));
        grid_->SetProperty("top", PixelValue(design_.gridY));

        for (std::size_t i = 0; i < slotElements_.size(); ++i)
        {
            Rml::Element* element = slotElements_[i];
            if (!element)
                continue;

            const std::size_t column = i % kColumns;
            const std::size_t row = i / kColumns;

            element->SetProperty(
                "left",
                PixelValue(
                    static_cast<float>(column) * design_.pitchX));
            element->SetProperty(
                "top",
                PixelValue(
                    static_cast<float>(row) * design_.pitchY));
            element->SetProperty(
                "width", PixelValue(design_.pitchX));
            element->SetProperty(
                "height", PixelValue(design_.pitchY));
        }
    }

    [[nodiscard]] std::array<RmlMuButton*, 7> Buttons()
    {
        return {
            &closeButton_, &exitButton_, &depositButton_,
            &withdrawButton_, &lockButton_, &extendButton_,
            &zenInfoButton_,
        };
    }

    RmlDocumentHost host_;
    Rml::ElementDocument* document_ = nullptr;

    Rml::Element* root_ = nullptr;
    Rml::Element* drag_ = nullptr;
    Rml::Element* title_ = nullptr;
    Rml::Element* grid_ = nullptr;
    Rml::Element* chargeGroup_ = nullptr;
    Rml::Element* periodGroup_ = nullptr;
    Rml::Element* storedZen_ = nullptr;
    Rml::Element* charge_ = nullptr;
    Rml::Element* remainingTime_ = nullptr;
    Rml::Element* lockElement_ = nullptr;
    Rml::Element* lockLabel_ = nullptr;

    std::array<Rml::Element*, kSlotCount> slotElements_{};
    std::array<RmlMuSlot, kSlotCount> slots_{};

    RmlMuButton closeButton_;
    RmlMuButton exitButton_;
    RmlMuButton depositButton_;
    RmlMuButton withdrawButton_;
    RmlMuButton lockButton_;
    RmlMuButton extendButton_;
    RmlMuButton zenInfoButton_;
    RmlMuMovablePanel mover_;

    Design design_;
    State state_;
    std::optional<Action> pendingAction_;
};

RmlStoragePanel::RmlStoragePanel()
    : m_impl(std::make_unique<Impl>())
{
}

RmlStoragePanel::~RmlStoragePanel() = default;
RmlStoragePanel::RmlStoragePanel(RmlStoragePanel&&) noexcept = default;
RmlStoragePanel& RmlStoragePanel::operator=(
    RmlStoragePanel&&) noexcept = default;

bool RmlStoragePanel::Load(bool show)
{
    return m_impl && m_impl->Load(show);
}

bool RmlStoragePanel::Show()
{
    return m_impl && m_impl->Show();
}

bool RmlStoragePanel::Hide()
{
    return m_impl && m_impl->Hide();
}

void RmlStoragePanel::Release()
{
    if (m_impl)
        m_impl->Release();
}

bool RmlStoragePanel::IsLoaded() const noexcept
{
    return m_impl && m_impl->IsLoaded();
}

bool RmlStoragePanel::ApplyState(const State& state)
{
    return m_impl && m_impl->ApplyState(state);
}

bool RmlStoragePanel::Update()
{
    return m_impl && m_impl->Update();
}

std::optional<RmlStoragePanel::Action>
RmlStoragePanel::ConsumeAction()
{
    return m_impl
        ? m_impl->ConsumeAction()
        : std::nullopt;
}

} // namespace UI::Modern::PC::Inventory

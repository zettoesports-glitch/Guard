#include "stdafx.h"
#include "UI/Modern/PC/Party/RmlPartyFrameLayer.h"

#include "UI/Modern/RmlDocumentHost.h"
#include "UI/Modern/RmlMuButton.h"
#include "UI/Modern/RmlMuMovablePanel.h"
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

namespace UI::Modern::PC::Party
{

namespace
{
constexpr const char* kDocumentPath =
    "Data/UI/PC/Party/party_frame.rml";

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

Rml::Element* ElementById(
    Rml::ElementDocument* document, const Rml::String& id)
{
    return document ? document->GetElementById(id) : nullptr;
}

Rml::String PixelValue(float value)
{
    std::array<char, 64> buffer{};
    std::snprintf(
        buffer.data(), buffer.size(), "%.3fpx",
        static_cast<double>(value));
    return Rml::String(buffer.data());
}

Rml::String PercentValue(float ratio)
{
    ratio = std::clamp(ratio, 0.0f, 1.0f);
    std::array<char, 64> buffer{};
    std::snprintf(
        buffer.data(), buffer.size(), "%.3f%%",
        static_cast<double>(ratio * 100.0f));
    return Rml::String(buffer.data());
}

void SetText(Rml::Element* element, const std::string& text)
{
    if (!element)
        return;

    element->SetInnerRML("");
    if (Rml::ElementDocument* document = element->GetOwnerDocument())
        element->AppendChild(document->CreateTextNode(text));
}

} // namespace

class RmlPartyFrameLayer::Impl
{
public:
    struct Design
    {
        float referenceWidth = 640.0f;
        float referenceHeight = 480.0f;
        float headerWidth = 147.0f;
        float headerHeight = 19.0f;
        float memberX = -1.0f;
        float memberWidth = 150.0f;
        float memberHeight = 45.0f;
        float firstRowY = 17.0f;
        float rowStep = 45.0f;
        float minimizeX = 132.0f;
        float minimizeY = 3.0f;
        float minimizeSize = 13.0f;
        float hpX = 25.0f;
        float hpY = 23.0f;
        float hpWidth = 120.0f;
        float hpHeight = 6.0f;
        float mpX = 25.0f;
        float mpY = 32.0f;
        float mpWidth = 120.0f;
        float mpHeight = 6.0f;
        float crownX = 0.95f;
        float crownY = 20.85f;
        float crownWidth = 24.0f;
        float crownHeight = 19.0f;
        float leaveX = 128.0f;
        float leaveY = 3.0f;
        float leaveSize = 18.0f;
        float backgroundHeight = 42.0f;
        float initialY = 38.0f;
    };

    Impl() : host_(kDocumentPath) {}
    ~Impl() { Release(); }

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

        hostVisible_ = show;
        return ApplyState(state_);
    }

    [[nodiscard]] bool Show()
    {
        if (!host_.IsLoaded() && !Load(false))
            return false;

        hostVisible_ = true;
        const bool result = host_.Show();
        if (result)
            (void)ApplyState(state_);
        return result;
    }

    [[nodiscard]] bool Hide()
    {
        hostVisible_ = false;
        if (root_)
            root_->SetProperty("display", "none");
        return host_.Hide();
    }

    void Release()
    {
        minimizeButton_.Unbind();
        for (auto& button : leaveButtons_) button.Unbind();
        for (auto& button : memberButtons_) button.Unbind();
        mover_.Unbind();

        root_ = dragbar_ = minimizeElement_ = nullptr;
        memberElements_.fill(nullptr);
        backgroundElements_.fill(nullptr);
        channelElements_.fill(nullptr);
        nameElements_.fill(nullptr);
        hpElements_.fill(nullptr);
        mpElements_.fill(nullptr);
        crownElements_.fill(nullptr);
        leaveElements_.fill(nullptr);
        overlayElements_.fill(nullptr);

        document_ = nullptr;
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
            hostVisible_ &&
            state_.visible &&
            state_.viewportWidth > 0 &&
            state_.viewportHeight > 0;

        root_->SetProperty("display", visible ? "block" : "none");
        if (!visible)
            return true;

        root_->SetClass("minimized", state_.minimized);
        ApplyLayout();

        for (std::size_t i = 0; i < MaxMembers; ++i)
        {
            const Member& member = state_.members[i];
            const bool shown = member.visible && !state_.minimized;

            memberElements_[i]->SetProperty(
                "display", shown ? "block" : "none");
            if (!shown)
            {
                memberButtons_[i].SetVisible(false);
                leaveButtons_[i].SetVisible(false);
                (void)memberButtons_[i].Update();
                (void)leaveButtons_[i].Update();
                continue;
            }

            SetText(channelElements_[i], member.channel);
            SetText(nameElements_[i], member.name);

            hpElements_[i]->SetProperty(
                "width", PercentValue(member.hpRatio));
            mpElements_[i]->SetProperty(
                "width", PercentValue(member.mpRatio));

            crownElements_[i]->SetProperty(
                "display", member.leader ? "block" : "none");

            memberElements_[i]->SetClass(
                "out-of-sight", member.outOfSight);
            memberElements_[i]->SetClass(
                "selected", member.selected);

            memberButtons_[i].SetVisible(true);
            memberButtons_[i].SetEnabled(true);
            leaveButtons_[i].SetVisible(member.canLeave);
            leaveButtons_[i].SetEnabled(member.canLeave);
            (void)memberButtons_[i].Update();
            (void)leaveButtons_[i].Update();
        }

        (void)minimizeButton_.Update();
        document_->UpdateDocument();
        return true;
    }

    [[nodiscard]] bool Update()
    {
        if (!IsLoaded())
            return false;

        bool changed = false;

        changed |= minimizeButton_.Update();
        if (minimizeButton_.ConsumeClicked())
        {
            pendingAction_ =
                Action{ActionType::ToggleMinimize, 0};
            changed = true;
        }

        for (std::size_t i = 0; i < MaxMembers; ++i)
        {
            changed |= memberButtons_[i].Update();
            changed |= leaveButtons_[i].Update();

            if (leaveButtons_[i].ConsumeClicked())
            {
                pendingAction_ =
                    Action{ActionType::LeaveMember, i};
                changed = true;
                continue;
            }

            if (memberButtons_[i].ConsumeClicked())
            {
                pendingAction_ =
                    Action{ActionType::SelectMember, i};
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
        const RmlUiDesign::Values values = LoadDesignValues();

        (void)ParseScalar(values, "PartyFrame-ReferenceWidth", design_.referenceWidth);
        (void)ParseScalar(values, "PartyFrame-ReferenceHeight", design_.referenceHeight);
        (void)ParseScalar(values, "PartyFrame-HeaderWidth", design_.headerWidth);
        (void)ParseScalar(values, "PartyFrame-HeaderHeight", design_.headerHeight);
        (void)ParseScalar(values, "PartyFrame-MemberX", design_.memberX);
        (void)ParseScalar(values, "PartyFrame-MemberWidth", design_.memberWidth);
        (void)ParseScalar(values, "PartyFrame-MemberHeight", design_.memberHeight);
        (void)ParseScalar(values, "PartyFrame-FirstRowY", design_.firstRowY);
        (void)ParseScalar(values, "PartyFrame-RowStep", design_.rowStep);
        (void)ParseScalar(values, "PartyFrame-MinimizeX", design_.minimizeX);
        (void)ParseScalar(values, "PartyFrame-MinimizeY", design_.minimizeY);
        (void)ParseScalar(values, "PartyFrame-MinimizeSize", design_.minimizeSize);
        (void)ParseScalar(values, "PartyFrame-HpX", design_.hpX);
        (void)ParseScalar(values, "PartyFrame-HpY", design_.hpY);
        (void)ParseScalar(values, "PartyFrame-HpWidth", design_.hpWidth);
        (void)ParseScalar(values, "PartyFrame-HpHeight", design_.hpHeight);
        (void)ParseScalar(values, "PartyFrame-MpX", design_.mpX);
        (void)ParseScalar(values, "PartyFrame-MpY", design_.mpY);
        (void)ParseScalar(values, "PartyFrame-MpWidth", design_.mpWidth);
        (void)ParseScalar(values, "PartyFrame-MpHeight", design_.mpHeight);
        (void)ParseScalar(values, "PartyFrame-CrownX", design_.crownX);
        (void)ParseScalar(values, "PartyFrame-CrownY", design_.crownY);
        (void)ParseScalar(values, "PartyFrame-CrownWidth", design_.crownWidth);
        (void)ParseScalar(values, "PartyFrame-CrownHeight", design_.crownHeight);
        (void)ParseScalar(values, "PartyFrame-LeaveX", design_.leaveX);
        (void)ParseScalar(values, "PartyFrame-LeaveY", design_.leaveY);
        (void)ParseScalar(values, "PartyFrame-LeaveSize", design_.leaveSize);
        (void)ParseScalar(values, "PartyFrame-BackgroundHeight", design_.backgroundHeight);
        (void)ParseScalar(values, "PartyFrame-InitialY", design_.initialY);
    }

    [[nodiscard]] bool BindElements()
    {
        root_ = ElementById(document_, "party-frame");
        dragbar_ = ElementById(document_, "party-dragbar");
        minimizeElement_ = ElementById(document_, "party-minimize");

        if (!root_ || !dragbar_ || !minimizeElement_)
            return false;

        minimizeButton_.Bind(minimizeElement_);

        for (std::size_t i = 0; i < MaxMembers; ++i)
        {
            const std::string suffix = std::to_string(i);

            memberElements_[i] =
                ElementById(document_, "party-member-" + suffix);
            backgroundElements_[i] =
                ElementById(document_, "party-background-" + suffix);
            channelElements_[i] =
                ElementById(document_, "party-channel-" + suffix);
            nameElements_[i] =
                ElementById(document_, "party-name-" + suffix);
            hpElements_[i] =
                ElementById(document_, "party-hp-" + suffix);
            mpElements_[i] =
                ElementById(document_, "party-mp-" + suffix);
            crownElements_[i] =
                ElementById(document_, "party-crown-" + suffix);
            leaveElements_[i] =
                ElementById(document_, "party-leave-" + suffix);
            overlayElements_[i] =
                ElementById(document_, "party-overlay-" + suffix);

            if (!memberElements_[i] || !backgroundElements_[i] ||
                !channelElements_[i] || !nameElements_[i] ||
                !hpElements_[i] || !mpElements_[i] ||
                !crownElements_[i] || !leaveElements_[i] ||
                !overlayElements_[i])
                return false;

            memberButtons_[i].Bind(overlayElements_[i]);
            leaveButtons_[i].Bind(leaveElements_[i]);
        }

        mover_.Bind(root_, dragbar_);
        mover_.SetPosition(0.0f, design_.initialY);
        return true;
    }

    void ApplyLayout()
    {
        const float memberCount = static_cast<float>(
            std::count_if(
                state_.members.begin(), state_.members.end(),
                [](const Member& member) { return member.visible; }));

        const float expandedHeight =
            design_.firstRowY +
            design_.rowStep * memberCount;
        const float height =
            state_.minimized ? design_.headerHeight :
            std::max(design_.headerHeight, expandedHeight);

        root_->SetProperty("width", PixelValue(design_.memberWidth));
        root_->SetProperty("height", PixelValue(height));

        minimizeElement_->SetProperty("left", PixelValue(design_.minimizeX));
        minimizeElement_->SetProperty("top", PixelValue(design_.minimizeY));
        minimizeElement_->SetProperty("width", PixelValue(design_.minimizeSize));
        minimizeElement_->SetProperty("height", PixelValue(design_.minimizeSize));

        for (std::size_t i = 0; i < MaxMembers; ++i)
        {
            const float y =
                design_.firstRowY +
                design_.rowStep * static_cast<float>(i);

            memberElements_[i]->SetProperty("left", PixelValue(design_.memberX));
            memberElements_[i]->SetProperty("top", PixelValue(y));
            memberElements_[i]->SetProperty("width", PixelValue(design_.memberWidth));
            memberElements_[i]->SetProperty("height", PixelValue(design_.memberHeight));

            hpElements_[i]->GetParentNode()->SetProperty("left", PixelValue(design_.hpX));
            hpElements_[i]->GetParentNode()->SetProperty("top", PixelValue(design_.hpY));
            hpElements_[i]->GetParentNode()->SetProperty("width", PixelValue(design_.hpWidth));
            hpElements_[i]->GetParentNode()->SetProperty("height", PixelValue(design_.hpHeight));

            mpElements_[i]->GetParentNode()->SetProperty("left", PixelValue(design_.mpX));
            mpElements_[i]->GetParentNode()->SetProperty("top", PixelValue(design_.mpY));
            mpElements_[i]->GetParentNode()->SetProperty("width", PixelValue(design_.mpWidth));
            mpElements_[i]->GetParentNode()->SetProperty("height", PixelValue(design_.mpHeight));

            crownElements_[i]->SetProperty("left", PixelValue(design_.crownX));
            crownElements_[i]->SetProperty("top", PixelValue(design_.crownY));
            crownElements_[i]->SetProperty("width", PixelValue(design_.crownWidth));
            crownElements_[i]->SetProperty("height", PixelValue(design_.crownHeight));

            leaveElements_[i]->SetProperty("left", PixelValue(design_.leaveX));
            leaveElements_[i]->SetProperty("top", PixelValue(design_.leaveY));
            leaveElements_[i]->SetProperty("width", PixelValue(design_.leaveSize));
            leaveElements_[i]->SetProperty("height", PixelValue(design_.leaveSize));
        }

        mover_.SetMetrics(
            static_cast<float>(state_.viewportWidth),
            static_cast<float>(state_.viewportHeight),
            design_.memberWidth,
            height,
            0.0f,
            0.0f);

        if (state_.applyPosition)
            mover_.SetPosition(state_.positionX, state_.positionY);
    }

    RmlDocumentHost host_;
    Rml::ElementDocument* document_ = nullptr;
    bool hostVisible_ = false;

    Rml::Element* root_ = nullptr;
    Rml::Element* dragbar_ = nullptr;
    Rml::Element* minimizeElement_ = nullptr;

    std::array<Rml::Element*, MaxMembers> memberElements_{};
    std::array<Rml::Element*, MaxMembers> backgroundElements_{};
    std::array<Rml::Element*, MaxMembers> channelElements_{};
    std::array<Rml::Element*, MaxMembers> nameElements_{};
    std::array<Rml::Element*, MaxMembers> hpElements_{};
    std::array<Rml::Element*, MaxMembers> mpElements_{};
    std::array<Rml::Element*, MaxMembers> crownElements_{};
    std::array<Rml::Element*, MaxMembers> leaveElements_{};
    std::array<Rml::Element*, MaxMembers> overlayElements_{};

    RmlMuButton minimizeButton_;
    std::array<RmlMuButton, MaxMembers> memberButtons_{};
    std::array<RmlMuButton, MaxMembers> leaveButtons_{};
    RmlMuMovablePanel mover_;

    Design design_;
    State state_;
    std::optional<Action> pendingAction_;
};

RmlPartyFrameLayer::RmlPartyFrameLayer()
    : m_impl(std::make_unique<Impl>())
{
}

RmlPartyFrameLayer::~RmlPartyFrameLayer() = default;
RmlPartyFrameLayer::RmlPartyFrameLayer(
    RmlPartyFrameLayer&&) noexcept = default;
RmlPartyFrameLayer& RmlPartyFrameLayer::operator=(
    RmlPartyFrameLayer&&) noexcept = default;

bool RmlPartyFrameLayer::Load(bool show)
{
    return m_impl && m_impl->Load(show);
}
bool RmlPartyFrameLayer::Show()
{
    return m_impl && m_impl->Show();
}
bool RmlPartyFrameLayer::Hide()
{
    return m_impl && m_impl->Hide();
}
void RmlPartyFrameLayer::Release()
{
    if (m_impl) m_impl->Release();
}
bool RmlPartyFrameLayer::IsLoaded() const noexcept
{
    return m_impl && m_impl->IsLoaded();
}
bool RmlPartyFrameLayer::ApplyState(const State& state)
{
    return m_impl && m_impl->ApplyState(state);
}
bool RmlPartyFrameLayer::Update()
{
    return m_impl && m_impl->Update();
}
std::optional<RmlPartyFrameLayer::Action>
RmlPartyFrameLayer::ConsumeAction()
{
    return m_impl ? m_impl->ConsumeAction() : std::nullopt;
}

} // namespace UI::Modern::PC::Party

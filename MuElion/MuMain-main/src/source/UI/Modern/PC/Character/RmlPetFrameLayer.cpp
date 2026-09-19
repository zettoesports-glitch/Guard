#include "stdafx.h"
#include "UI/Modern/PC/Character/RmlPetFrameLayer.h"

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

namespace UI::Modern::PC::Character
{

namespace
{
constexpr const char* kDocumentPath =
    "Data/UI/PC/Character/pet_frame.rml";

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
    std::array<char, 64> buffer{};
    std::snprintf(
        buffer.data(), buffer.size(), "%.3f%%",
        static_cast<double>(
            std::clamp(ratio, 0.0f, 1.0f) * 100.0f));
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
    Rml::ElementDocument* document, const Rml::String& id)
{
    return document ? document->GetElementById(id) : nullptr;
}

} // namespace

class RmlPetFrameLayer::Impl
{
public:
    struct Design
    {
        float dragWidth = 100.0f;
        float dragHeight = 19.0f;
        float memberHeight = 33.0f;
        float referenceWidth = 640.0f;
        float referenceHeight = 480.0f;
        float minimizeX = 82.0f;
        float minimizeY = 3.0f;
        float minimizeWidth = 13.0f;
        float minimizeHeight = 13.0f;
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
        minimizeButton_.Unbind();
        mover_.Unbind();

        document_ = nullptr;
        root_ = nullptr;
        dragBar_ = nullptr;
        minimizeElement_ = nullptr;
        memberElements_.fill(nullptr);
        nameElements_.fill(nullptr);
        healthElements_.fill(nullptr);
        yellowHealthElements_.fill(nullptr);
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

        std::size_t visibleCount = 0;
        for (std::size_t i = 0; i < state_.members.size(); ++i)
        {
            const Member& member = state_.members[i];
            const bool memberVisible =
                !state_.minimized && member.visible;

            memberElements_[i]->SetProperty(
                "display", memberVisible ? "block" : "none");

            if (!memberVisible)
                continue;

            ++visibleCount;
            nameElements_[i]->SetInnerRML(member.name);
            healthElements_[i]->SetProperty(
                "width", PercentValue(member.healthRatio));

            yellowHealthElements_[i]->SetProperty(
                "display",
                member.showYellowHealth ? "block" : "none");
            yellowHealthElements_[i]->SetProperty(
                "width",
                PercentValue(member.yellowHealthRatio));
        }

        const float height =
            design_.dragHeight +
            (state_.minimized
                ? 0.0f
                : design_.memberHeight *
                    static_cast<float>(visibleCount));
        root_->SetProperty(
            "height", PixelValue(std::max(
                design_.dragHeight, height)));

        minimizeElement_->SetClass(
            "minimized", state_.minimized);

        document_->UpdateDocument();
        return true;
    }

    [[nodiscard]] bool Update()
    {
        if (!IsLoaded())
            return false;

        bool changed = minimizeButton_.Update();

        if (minimizeButton_.ConsumeClicked())
        {
            state_.minimized = !state_.minimized;
            pendingAction_ = Action::ToggleMinimized;
            (void)ApplyState(state_);
            changed = true;
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
        (void)ParseScalar(
            values, "RmlPetFrameLayer-DragWidth",
            design_.dragWidth);
        (void)ParseScalar(
            values, "RmlPetFrameLayer-DragHeight",
            design_.dragHeight);
        (void)ParseScalar(
            values, "RmlPetFrameLayer-MemberHeight",
            design_.memberHeight);
        (void)ParseScalar(
            values, "RmlPetFrameLayer-ReferenceWidth",
            design_.referenceWidth);
        (void)ParseScalar(
            values, "RmlPetFrameLayer-ReferenceHeight",
            design_.referenceHeight);
        (void)ParseScalar(
            values, "RmlPetFrameLayer-MinimizeX",
            design_.minimizeX);
        (void)ParseScalar(
            values, "RmlPetFrameLayer-MinimizeY",
            design_.minimizeY);
        (void)ParseScalar(
            values, "RmlPetFrameLayer-MinimizeWidth",
            design_.minimizeWidth);
        (void)ParseScalar(
            values, "RmlPetFrameLayer-MinimizeHeight",
            design_.minimizeHeight);
    }

    [[nodiscard]] bool BindElements()
    {
        root_ = RequiredElement(document_, "pet-frame");
        dragBar_ = RequiredElement(document_, "pet-dragbar");
        minimizeElement_ =
            RequiredElement(document_, "pet-minimize");

        if (!root_ || !dragBar_ || !minimizeElement_)
            return false;

        for (std::size_t i = 0; i < memberElements_.size(); ++i)
        {
            const std::string suffix = std::to_string(i);
            memberElements_[i] =
                RequiredElement(document_, "pet-member-" + suffix);
            nameElements_[i] =
                RequiredElement(document_, "pet-name-" + suffix);
            healthElements_[i] =
                RequiredElement(document_, "pet-hp-" + suffix);

            if (!memberElements_[i] ||
                !nameElements_[i] ||
                !healthElements_[i])
                return false;

            if (memberElements_[i]->GetNumChildren() < 2)
                return false;

            Rml::Element* track = memberElements_[i]->GetChild(1);
            if (!track || track->GetNumChildren() < 2)
                return false;

            yellowHealthElements_[i] = track->GetChild(1);
            if (!yellowHealthElements_[i])
                return false;
        }

        minimizeButton_.Bind(minimizeElement_);
        mover_.Bind(root_, dragBar_);
        return true;
    }

    void ApplyLayout()
    {
        const float width = design_.dragWidth;
        const float maximumHeight =
            design_.dragHeight +
            design_.memberHeight *
                static_cast<float>(state_.members.size());

        root_->SetProperty("width", PixelValue(width));
        dragBar_->SetProperty("width", PixelValue(design_.dragWidth));
        dragBar_->SetProperty("height", PixelValue(design_.dragHeight));

        minimizeElement_->SetProperty(
            "left", PixelValue(design_.minimizeX));
        minimizeElement_->SetProperty(
            "top", PixelValue(design_.minimizeY));
        minimizeElement_->SetProperty(
            "width", PixelValue(design_.minimizeWidth));
        minimizeElement_->SetProperty(
            "height", PixelValue(design_.minimizeHeight));

        for (std::size_t i = 0; i < memberElements_.size(); ++i)
        {
            memberElements_[i]->SetProperty(
                "top",
                PixelValue(
                    design_.dragHeight +
                    design_.memberHeight *
                        static_cast<float>(i)));
            memberElements_[i]->SetProperty(
                "height", PixelValue(design_.memberHeight));
        }

        mover_.SetMetrics(
            static_cast<float>(state_.viewportWidth),
            static_cast<float>(state_.viewportHeight),
            width,
            maximumHeight,
            0.0f,
            0.0f);

        if (state_.applyPosition)
            mover_.SetPosition(
                state_.positionX, state_.positionY);
    }

    RmlDocumentHost host_;
    Rml::ElementDocument* document_ = nullptr;

    Rml::Element* root_ = nullptr;
    Rml::Element* dragBar_ = nullptr;
    Rml::Element* minimizeElement_ = nullptr;

    std::array<Rml::Element*, 5> memberElements_{};
    std::array<Rml::Element*, 5> nameElements_{};
    std::array<Rml::Element*, 5> healthElements_{};
    std::array<Rml::Element*, 5> yellowHealthElements_{};

    RmlMuButton minimizeButton_;
    RmlMuMovablePanel mover_;

    Design design_;
    State state_;
    std::optional<Action> pendingAction_;
};

RmlPetFrameLayer::RmlPetFrameLayer()
    : m_impl(std::make_unique<Impl>())
{
}

RmlPetFrameLayer::~RmlPetFrameLayer() = default;
RmlPetFrameLayer::RmlPetFrameLayer(
    RmlPetFrameLayer&&) noexcept = default;
RmlPetFrameLayer& RmlPetFrameLayer::operator=(
    RmlPetFrameLayer&&) noexcept = default;

bool RmlPetFrameLayer::Load(bool show)
{
    return m_impl && m_impl->Load(show);
}

bool RmlPetFrameLayer::Show()
{
    return m_impl && m_impl->Show();
}

bool RmlPetFrameLayer::Hide()
{
    return m_impl && m_impl->Hide();
}

void RmlPetFrameLayer::Release()
{
    if (m_impl)
        m_impl->Release();
}

bool RmlPetFrameLayer::IsLoaded() const noexcept
{
    return m_impl && m_impl->IsLoaded();
}

bool RmlPetFrameLayer::ApplyState(const State& state)
{
    return m_impl && m_impl->ApplyState(state);
}

bool RmlPetFrameLayer::Update()
{
    return m_impl && m_impl->Update();
}

std::optional<RmlPetFrameLayer::Action>
RmlPetFrameLayer::ConsumeAction()
{
    return m_impl
        ? m_impl->ConsumeAction()
        : std::nullopt;
}

} // namespace UI::Modern::PC::Character

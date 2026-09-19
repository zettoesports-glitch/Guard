#include "stdafx.h"
#include "UI/Modern/PC/Gens/RmlGensRankingPanel.h"

#include "UI/Modern/RmlDocumentHost.h"
#include "UI/Modern/RmlMuButton.h"
#include "UI/Modern/RmlMuMovablePanel.h"
#include "UI/Modern/RmlUiDesign.h"

#include <RmlUi/Core/Core.h>
#include <RmlUi/Core/Element.h>
#include <RmlUi/Core/ElementDocument.h>
#include <RmlUi/Core/FileInterface.h>
#include <RmlUi/Core/StreamMemory.h>

#include <array>
#include <cstdio>
#include <sstream>
#include <utility>

namespace UI::Modern::PC::Gens
{
namespace
{
constexpr const char* kDocumentPath =
    "Data/UI/PC/Gens/gens_ranking.rml";

Rml::String PixelValue(float value)
{
    std::array<char, 64> buffer{};
    std::snprintf(
        buffer.data(), buffer.size(), "%.3fpx",
        static_cast<double>(value));
    return Rml::String(buffer.data());
}

bool ParsePair(
    const RmlUiDesign::Values& values,
    const char* key,
    std::array<float, 2>& output)
{
    const auto it = values.find(key);
    if (it == values.end())
        return false;
    std::istringstream stream(it->second);
    return static_cast<bool>(stream >> output[0] >> output[1]);
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
    Rml::ElementDocument* document,
    const char* id)
{
    return document ? document->GetElementById(id) : nullptr;
}

void SetText(Rml::Element* element, const std::string& text)
{
    if (!element)
        return;

    element->SetInnerRML("");
    if (Rml::ElementDocument* document = element->GetOwnerDocument())
        element->AppendChild(document->CreateTextNode(text));
}

void SetVisible(Rml::Element* element, bool visible)
{
    if (element)
        element->SetProperty("display", visible ? "block" : "none");
}
} // namespace

class RmlGensRankingPanel::Impl
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

        const auto values = LoadDesignValues();
        (void)ParsePair(values, "Panel-Size", panelSize_);
        (void)ParsePair(values, "Panel-Reference", referenceSize_);
        (void)ParsePair(values, "Panel-Initial", initialPosition_);

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
        const bool ok = host_.Show();
        if (ok)
            (void)ApplyState(state_);
        return ok;
    }

    [[nodiscard]] bool Hide()
    {
        hostVisible_ = false;
        SetVisible(panel_, false);
        return host_.Hide();
    }

    void Release()
    {
        close_.Unbind();
        mover_.Unbind();

        panel_ = drag_ = nullptr;
        title_ = gensInfoMent_ = gensInfo_ = gensState_ = nullptr;
        gensTeam_ = gensRanking_ = ranking_ = contribution_ = nullptr;
        gensTeamValue_ = gensRankingValue_ = rankingValue_ = nullptr;
        grade_ = contributionValue_ = nullptr;
        markV_ = markD_ = nullptr;

        document_ = nullptr;
        pending_.reset();
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

        SetVisible(panel_, visible);
        if (!visible)
            return true;

        mover_.SetMetrics(
            static_cast<float>(state_.viewportWidth),
            static_cast<float>(state_.viewportHeight),
            panelSize_[0], panelSize_[1], 0.0f, 0.0f);
        mover_.SetPosition(state_.x, state_.y);

        SetText(title_, state_.title);
        SetText(gensInfo_, state_.gensInfoLabel);
        SetText(gensTeam_, state_.gensTeamLabel);
        SetText(gensRanking_, state_.gensRankingLabel);
        SetText(ranking_, state_.rankingLabel);
        SetText(contribution_, state_.contributionLabel);

        SetText(gensTeamValue_, state_.teamValue);
        SetText(gensRankingValue_, state_.gradeValue);
        SetText(rankingValue_, state_.rankingValue);
        SetText(grade_, state_.gradeValue);
        SetText(contributionValue_, state_.contributionValue);
        SetText(gensState_, state_.promotionText);

        std::string infoText;
        for (std::size_t i = 0; i < state_.infoLines.size(); ++i)
        {
            if (i != 0)
                infoText += "\n";
            infoText += state_.infoLines[i];
        }
        SetText(gensInfoMent_, infoText);

        SetVisible(markD_, state_.influence == Influence::Duprian);
        SetVisible(markV_, state_.influence == Influence::Vanert);

        const int rank = state_.rankIndex >= 1 && state_.rankIndex <= 14
            ? state_.rankIndex
            : 14;
        if (markD_)
            markD_->SetClass(
                "rank-" + std::to_string(rank),
                state_.influence == Influence::Duprian);
        if (markV_)
            markV_->SetClass(
                "rank-" + std::to_string(rank),
                state_.influence == Influence::Vanert);

        (void)close_.Update();
        document_->UpdateDocument();
        return true;
    }

    [[nodiscard]] bool Update()
    {
        if (!IsLoaded())
            return false;

        bool changed = close_.Update();
        if (close_.ConsumeClicked())
        {
            pending_ = Action::Close;
            changed = true;
        }

        changed |= mover_.ConsumePositionChanged();
        return changed;
    }

    [[nodiscard]] std::optional<Action> ConsumeAction()
    {
        return std::exchange(pending_, std::nullopt);
    }

private:
    [[nodiscard]] bool BindElements()
    {
        panel_ = ElementById(document_, "panel");
        drag_ = ElementById(document_, "btnDrag");
        title_ = ElementById(document_, "tfTitle");
        gensInfoMent_ = ElementById(document_, "taGensInfoMent");
        gensInfo_ = ElementById(document_, "tfGensInfo");
        gensState_ = ElementById(document_, "taGensState");
        gensTeam_ = ElementById(document_, "tfGensTeam");
        gensRanking_ = ElementById(document_, "tfGensRanking");
        ranking_ = ElementById(document_, "tfRanking");
        contribution_ = ElementById(document_, "tfContribution");
        gensTeamValue_ = ElementById(document_, "tfGensTeamValue");
        gensRankingValue_ = ElementById(document_, "tfGensRankingValue");
        rankingValue_ = ElementById(document_, "tfRankingValue");
        grade_ = ElementById(document_, "tfGrade");
        contributionValue_ = ElementById(document_, "tfContributionValue");
        markV_ = ElementById(document_, "mcMark_V");
        markD_ = ElementById(document_, "mcMark_D");
        Rml::Element* close = ElementById(document_, "btnClose");

        if (!panel_ || !drag_ || !title_ || !gensInfoMent_ ||
            !gensInfo_ || !gensState_ || !gensTeam_ ||
            !gensRanking_ || !ranking_ || !contribution_ ||
            !gensTeamValue_ || !gensRankingValue_ ||
            !rankingValue_ || !grade_ || !contributionValue_ ||
            !markV_ || !markD_ || !close)
            return false;

        close_.Bind(close);
        mover_.Bind(panel_, drag_);
        return true;
    }

    RmlDocumentHost host_;
    Rml::ElementDocument* document_ = nullptr;
    bool hostVisible_ = false;

    Rml::Element* panel_ = nullptr;
    Rml::Element* drag_ = nullptr;
    Rml::Element* title_ = nullptr;
    Rml::Element* gensInfoMent_ = nullptr;
    Rml::Element* gensInfo_ = nullptr;
    Rml::Element* gensState_ = nullptr;
    Rml::Element* gensTeam_ = nullptr;
    Rml::Element* gensRanking_ = nullptr;
    Rml::Element* ranking_ = nullptr;
    Rml::Element* contribution_ = nullptr;
    Rml::Element* gensTeamValue_ = nullptr;
    Rml::Element* gensRankingValue_ = nullptr;
    Rml::Element* rankingValue_ = nullptr;
    Rml::Element* grade_ = nullptr;
    Rml::Element* contributionValue_ = nullptr;
    Rml::Element* markV_ = nullptr;
    Rml::Element* markD_ = nullptr;

    RmlMuButton close_;
    RmlMuMovablePanel mover_;

    std::array<float, 2> panelSize_{327.0f, 639.0f};
    std::array<float, 2> referenceSize_{640.0f, 480.0f};
    std::array<float, 2> initialPosition_{20.0f, 20.0f};

    State state_;
    std::optional<Action> pending_;
};

RmlGensRankingPanel::RmlGensRankingPanel()
    : m_impl(std::make_unique<Impl>()) {}
RmlGensRankingPanel::~RmlGensRankingPanel() = default;
RmlGensRankingPanel::RmlGensRankingPanel(RmlGensRankingPanel&&) noexcept = default;
RmlGensRankingPanel& RmlGensRankingPanel::operator=(RmlGensRankingPanel&&) noexcept = default;
bool RmlGensRankingPanel::Load(bool show) { return m_impl && m_impl->Load(show); }
bool RmlGensRankingPanel::Show() { return m_impl && m_impl->Show(); }
bool RmlGensRankingPanel::Hide() { return m_impl && m_impl->Hide(); }
void RmlGensRankingPanel::Release() { if (m_impl) m_impl->Release(); }
bool RmlGensRankingPanel::IsLoaded() const noexcept { return m_impl && m_impl->IsLoaded(); }
bool RmlGensRankingPanel::ApplyState(const State& state) { return m_impl && m_impl->ApplyState(state); }
bool RmlGensRankingPanel::Update() { return m_impl && m_impl->Update(); }
std::optional<RmlGensRankingPanel::Action> RmlGensRankingPanel::ConsumeAction()
{
    return m_impl ? m_impl->ConsumeAction() : std::nullopt;
}

} // namespace UI::Modern::PC::Gens

#include "stdafx.h"
#include "UI/Modern/PC/Help/RmlHelpPanel.h"

#include "UI/Modern/RmlDocumentHost.h"
#include "UI/Modern/RmlMuButton.h"
#include "UI/Modern/RmlMuMovablePanel.h"
#include "UI/Modern/RmlMuScrollBar.h"
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

namespace UI::Modern::PC::Help
{
namespace
{
constexpr const char* kDocumentPath = "Data/UI/PC/Help/help.rml";
constexpr std::size_t kVisibleRows = 14;

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
    Rml::ElementDocument* document, const char* id)
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

class RmlHelpPanel::Impl
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
        closeButton_.Unbind();
        for (auto& button : tabButtons_)
            button.Unbind();
        for (auto& scroll : scrollbars_)
            scroll.Unbind();
        mover_.Unbind();

        panel_ = drag_ = title_ = nullptr;
        headings_.fill(nullptr);
        descriptions_.fill(nullptr);
        lists_.fill(nullptr);
        pages_.fill(nullptr);
        tabLabels_.fill(nullptr);
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
        if (state_.selectedPage >= PageCount)
            state_.selectedPage = 0;

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

        SetText(title_, state_.title);
        for (std::size_t i = 0; i < PageCount; ++i)
        {
            SetText(tabLabels_[i], state_.tabLabels[i]);
            tabButtons_[i].SetEnabled(true);
            (void)tabButtons_[i].Update();

            SetVisible(pages_[i], i == state_.selectedPage);
            tabLabels_[i]->GetParentNode()->SetClass(
                "selected", i == state_.selectedPage);
            SetText(headings_[i], state_.pages[i].heading);
        }

        mover_.SetMetrics(
            static_cast<float>(state_.viewportWidth),
            static_cast<float>(state_.viewportHeight),
            panelSize_[0], panelSize_[1], 0.0f, 0.0f);
        mover_.SetPosition(state_.x, state_.y);

        ApplyPage(0);
        ApplyPage(1);

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

        for (std::size_t i = 0; i < PageCount; ++i)
        {
            changed |= tabButtons_[i].Update();
            if (tabButtons_[i].ConsumeClicked())
            {
                pendingAction_ = Action{ActionType::SelectPage, i};
                changed = true;
            }

            if (const auto request =
                    scrollbars_[i].ConsumeRequestedPosition())
            {
                scrollStarts_[i] = *request;
                ApplyPage(i);
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
    [[nodiscard]] bool BindElements()
    {
        panel_ = ElementById(document_, "panel");
        drag_ = ElementById(document_, "btnDrag");
        title_ = ElementById(document_, "tfTitle");
        if (!panel_ || !drag_ || !title_)
            return false;

        Rml::Element* close = ElementById(document_, "btnClose");
        Rml::Element* hot = ElementById(document_, "btnHotKey");
        Rml::Element* chat = ElementById(document_, "btnChatHotKey");
        tabLabels_[0] = ElementById(document_, "btnHotKey-label");
        tabLabels_[1] = ElementById(document_, "btnChatHotKey-label");
        pages_[0] = ElementById(document_, "mcHotKey");
        pages_[1] = ElementById(document_, "mcChatHotKey");
        headings_[0] = ElementById(document_, "tfFunction");
        headings_[1] = ElementById(document_, "tfChatFunction");
        descriptions_[0] = ElementById(document_, "tfFunctionMent");
        descriptions_[1] = ElementById(document_, "tfChatFunctionMent");
        lists_[0] = ElementById(document_, "scrollingHotKey");
        lists_[1] = ElementById(document_, "scrollingChatKey");

        if (!close || !hot || !chat ||
            !tabLabels_[0] || !tabLabels_[1] ||
            !pages_[0] || !pages_[1] ||
            !headings_[0] || !headings_[1] ||
            !descriptions_[0] || !descriptions_[1] ||
            !lists_[0] || !lists_[1])
            return false;

        closeButton_.Bind(close);
        tabButtons_[0].Bind(hot);
        tabButtons_[1].Bind(chat);
        mover_.Bind(panel_, drag_);

        if (!scrollbars_[0].Bind(ElementById(document_, "sbHotKey")) ||
            !scrollbars_[1].Bind(ElementById(document_, "sbChatHotKey")))
            return false;

        return true;
    }

    void ApplyPage(std::size_t page)
    {
        if (page >= PageCount || !lists_[page])
            return;

        const auto& rows = state_.pages[page].rows;
        const std::size_t maximum =
            rows.size() > kVisibleRows ? rows.size() - kVisibleRows : 0u;
        scrollStarts_[page] =
            std::min(scrollStarts_[page], maximum);

        lists_[page]->SetInnerRML("");
        const std::size_t end = std::min(
            rows.size(), scrollStarts_[page] + kVisibleRows);

        for (std::size_t i = scrollStarts_[page]; i < end; ++i)
        {
            Rml::ElementPtr row = document_->CreateElement("div");
            row->SetClass("help-row", true);
            row->AppendChild(document_->CreateTextNode(rows[i]));
            lists_[page]->AppendChild(std::move(row));
        }

        (void)scrollbars_[page].SetState(
            scrollStarts_[page],
            maximum,
            kVisibleRows,
            kVisibleRows,
            maximum > 0,
            true);
    }

    RmlDocumentHost host_;
    Rml::ElementDocument* document_ = nullptr;
    bool hostVisible_ = false;

    Rml::Element* panel_ = nullptr;
    Rml::Element* drag_ = nullptr;
    Rml::Element* title_ = nullptr;
    std::array<Rml::Element*, PageCount> pages_{};
    std::array<Rml::Element*, PageCount> headings_{};
    std::array<Rml::Element*, PageCount> descriptions_{};
    std::array<Rml::Element*, PageCount> lists_{};
    std::array<Rml::Element*, PageCount> tabLabels_{};

    RmlMuButton closeButton_;
    std::array<RmlMuButton, PageCount> tabButtons_{};
    std::array<RmlMuScrollBar, PageCount> scrollbars_{};
    RmlMuMovablePanel mover_;

    std::array<float, 2> panelSize_{327.0f, 639.0f};
    std::array<float, 2> referenceSize_{640.0f, 480.0f};
    std::array<float, 2> initialPosition_{20.0f, 20.0f};
    std::array<std::size_t, PageCount> scrollStarts_{};

    State state_;
    std::optional<Action> pendingAction_;
};

RmlHelpPanel::RmlHelpPanel()
    : m_impl(std::make_unique<Impl>())
{
}
RmlHelpPanel::~RmlHelpPanel() = default;
RmlHelpPanel::RmlHelpPanel(RmlHelpPanel&&) noexcept = default;
RmlHelpPanel& RmlHelpPanel::operator=(RmlHelpPanel&&) noexcept = default;

bool RmlHelpPanel::Load(bool show) { return m_impl && m_impl->Load(show); }
bool RmlHelpPanel::Show() { return m_impl && m_impl->Show(); }
bool RmlHelpPanel::Hide() { return m_impl && m_impl->Hide(); }
void RmlHelpPanel::Release() { if (m_impl) m_impl->Release(); }
bool RmlHelpPanel::IsLoaded() const noexcept { return m_impl && m_impl->IsLoaded(); }
bool RmlHelpPanel::ApplyState(const State& state) { return m_impl && m_impl->ApplyState(state); }
bool RmlHelpPanel::Update() { return m_impl && m_impl->Update(); }
std::optional<RmlHelpPanel::Action> RmlHelpPanel::ConsumeAction()
{
    return m_impl ? m_impl->ConsumeAction() : std::nullopt;
}

} // namespace UI::Modern::PC::Help

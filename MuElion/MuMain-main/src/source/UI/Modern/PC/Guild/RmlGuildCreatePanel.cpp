#include "stdafx.h"
#include "UI/Modern/PC/Guild/RmlGuildCreatePanel.h"

#include "UI/Modern/RmlDocumentHost.h"
#include "UI/Modern/RmlMuButton.h"
#include "UI/Modern/RmlMuMovablePanel.h"
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

namespace UI::Modern::PC::Guild
{
namespace
{
constexpr const char* kDocumentPath = "Data/UI/PC/Guild/guild_create.rml";

Rml::Element* ElementById(Rml::ElementDocument* document, const char* id)
{
    return document ? document->GetElementById(id) : nullptr;
}

void SetVisible(Rml::Element* element, bool visible)
{
    if (element)
        element->SetProperty("display", visible ? "block" : "none");
}

void SetText(Rml::Element* element, const std::string& text)
{
    if (!element)
        return;
    element->SetInnerRML("");
    if (auto* document = element->GetOwnerDocument())
        element->AppendChild(document->CreateTextNode(text));
}

bool ParsePair(
    const RmlUiDesign::Values& values, const char* key,
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
        reinterpret_cast<const Rml::byte*>(contents.data()), contents.size());
    return RmlUiDesign::Parse(&stream);
}

Rml::String IndexedId(const char* prefix, std::size_t index)
{
    return Rml::String(prefix) + std::to_string(index + 1u);
}

Rml::String PaletteClass(std::uint8_t color)
{
    return Rml::String("palette-color-") +
           std::to_string(static_cast<unsigned>(color & 0x0fu));
}
} // namespace

class RmlGuildCreatePanel::Impl
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

        const auto design = LoadDesignValues();
        (void)ParsePair(design, "Panel-Size", panelSize_);
        (void)ParsePair(design, "Panel-Reference", referenceSize_);
        (void)ParsePair(design, "Panel-Initial", initialPosition_);

        if (!Bind())
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
        SetVisible(panel_, false);
        return host_.Hide();
    }

    void Release()
    {
        close_.Unbind();
        next_.Unbind();
        previous_.Unbind();
        confirm_.Unbind();
        for (auto& button : colors_)
            button.Unbind();
        for (auto& button : markCells_)
            button.Unbind();
        mover_.Unbind();

        document_ = nullptr;
        panel_ = drag_ = title_ = nullptr;
        intro_ = markPage_ = confirmPage_ = nullptr;
        markGrid_ = previewGrid_ = colorContainer_ = nullptr;
        checkName_ = checkMaster_ = nullptr;
        input_ = nullptr;
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
        state_.selectedColor &= 0x0fu;
        for (auto& value : state_.mark)
            value &= 0x0fu;

        if (!IsLoaded())
            return false;

        const bool visible = hostVisible_ && state_.visible &&
            state_.viewportWidth > 0 && state_.viewportHeight > 0;
        SetVisible(panel_, visible);
        if (!visible)
            return true;

        SetText(title_, state_.title);
        input_->SetValue(state_.guildName);
        input_->SetAttribute("maxlength", 8);

        SetVisible(intro_, state_.page == Page::Intro);
        SetVisible(markPage_, state_.page == Page::Mark);
        SetVisible(confirmPage_, state_.page == Page::Confirm);

        previous_.SetVisible(state_.page != Page::Intro);
        next_.SetVisible(state_.page != Page::Confirm);
        confirm_.SetVisible(state_.page == Page::Confirm);
        (void)previous_.Update();
        (void)next_.Update();
        (void)confirm_.Update();

        SetText(checkName_, state_.guildName);
        SetText(checkMaster_, state_.guildMaster);

        mover_.SetMetrics(
            static_cast<float>(state_.viewportWidth),
            static_cast<float>(state_.viewportHeight),
            panelSize_[0], panelSize_[1], 0.0f, 0.0f);
        mover_.SetPosition(state_.x, state_.y);

        ApplyPalette();
        document_->UpdateDocument();
        return true;
    }

    [[nodiscard]] bool Update()
    {
        if (!IsLoaded())
            return false;

        bool changed = close_.Update();
        changed |= next_.Update();
        changed |= previous_.Update();
        changed |= confirm_.Update();

        if (close_.ConsumeClicked())
        {
            Queue(ActionType::Close);
            changed = true;
        }
        if (previous_.ConsumeClicked())
        {
            if (state_.page == Page::Confirm)
                state_.page = Page::Mark;
            else if (state_.page == Page::Mark)
                state_.page = Page::Intro;
            Queue(ActionType::Previous);
            (void)ApplyState(state_);
            changed = true;
        }
        if (next_.ConsumeClicked())
        {
            PullName();
            if (state_.page == Page::Intro)
                state_.page = Page::Mark;
            else if (state_.page == Page::Mark)
                state_.page = Page::Confirm;
            Queue(ActionType::Next);
            (void)ApplyState(state_);
            changed = true;
        }
        if (confirm_.ConsumeClicked())
        {
            PullName();
            Queue(ActionType::Confirm);
            changed = true;
        }

        for (std::size_t i = 0; i < colors_.size(); ++i)
        {
            changed |= colors_[i].Update();
            if (!colors_[i].ConsumeClicked())
                continue;

            state_.selectedColor = static_cast<std::uint8_t>(i);
            Queue(ActionType::SelectColor, i, state_.selectedColor);
            ApplyPalette();
            changed = true;
        }

        for (std::size_t i = 0; i < markCells_.size(); ++i)
        {
            changed |= markCells_[i].Update();
            if (!markCells_[i].ConsumeClicked())
                continue;

            state_.mark[i] = state_.selectedColor;
            Queue(ActionType::PaintCell, i, state_.selectedColor);
            ApplyPalette();
            changed = true;
        }

        changed |= mover_.ConsumePositionChanged();
        return changed;
    }

    [[nodiscard]] std::optional<Action> ConsumeAction()
    {
        return std::exchange(pending_, std::nullopt);
    }

    [[nodiscard]] const State& GetState() const noexcept { return state_; }

private:
    [[nodiscard]] bool Bind()
    {
        panel_ = ElementById(document_, "panel");
        drag_ = ElementById(document_, "btnDrag");
        title_ = ElementById(document_, "tfTitle");
        intro_ = ElementById(document_, "mcInfo");
        markPage_ = ElementById(document_, "mcGuildMark");
        confirmPage_ = ElementById(document_, "mcGuildCheck");
        markGrid_ = ElementById(document_, "mcGuildMarkDraw");
        previewGrid_ = ElementById(document_, "mcCheckGuildMark");
        colorContainer_ = ElementById(document_, "guild-colors");
        checkName_ = ElementById(document_, "tfCheckGuildNameValue");
        checkMaster_ = ElementById(document_, "tfCheckGuildMaster");
        input_ = dynamic_cast<Rml::ElementFormControlInput*>(
            ElementById(document_, "tiGuildNameValue"));

        Rml::Element* close = ElementById(document_, "btnClose");
        Rml::Element* next = ElementById(document_, "btnNext");
        Rml::Element* previous = ElementById(document_, "btnPrev");
        Rml::Element* confirm = ElementById(document_, "btnConfirm");

        if (!panel_ || !drag_ || !title_ || !intro_ || !markPage_ ||
            !confirmPage_ || !markGrid_ || !previewGrid_ ||
            !colorContainer_ || !checkName_ || !checkMaster_ || !input_ ||
            !close || !next || !previous || !confirm)
            return false;

        close_.Bind(close);
        next_.Bind(next);
        previous_.Bind(previous);
        confirm_.Bind(confirm);
        mover_.Bind(panel_, drag_);

        BuildPaletteElements();
        return true;
    }

    void BuildPaletteElements()
    {
        markGrid_->SetInnerRML("");
        previewGrid_->SetInnerRML("");
        colorContainer_->SetInnerRML("");

        for (std::size_t i = 0; i < MarkCellCount; ++i)
        {
            Rml::ElementPtr cell = document_->CreateElement("div");
            cell->SetId(IndexedId("mcGuildMarkDraw-mcBlock", i));
            cell->SetClass("guild-mark-cell", true);
            Rml::Element* raw = markGrid_->AppendChild(std::move(cell));
            markCells_[i].Bind(raw);

            Rml::ElementPtr preview = document_->CreateElement("div");
            preview->SetId(IndexedId("mcCheckGuildMark-mcBlock", i));
            preview->SetClass("guild-mark-cell", true);
            preview->SetClass("read-only", true);
            previewGrid_->AppendChild(std::move(preview));
        }

        for (std::size_t i = 0; i < ColorCount; ++i)
        {
            Rml::ElementPtr swatch = document_->CreateElement("div");
            swatch->SetId(IndexedId("btnColor", i));
            swatch->SetClass("mu-button", true);
            swatch->SetClass("guild-color", true);
            swatch->SetClass(
                PaletteClass(static_cast<std::uint8_t>(i)), true);
            Rml::Element* raw = colorContainer_->AppendChild(std::move(swatch));
            colors_[i].Bind(raw);
        }
    }

    void PullName()
    {
        if (input_)
            state_.guildName = input_->GetValue();
    }

    void ApplyPalette()
    {
        if (!document_)
            return;

        for (std::size_t i = 0; i < MarkCellCount; ++i)
        {
            const auto color = state_.mark[i] & 0x0fu;
            if (auto* cell = ElementById(
                    document_, IndexedId("mcGuildMarkDraw-mcBlock", i).c_str()))
            {
                for (std::uint8_t c = 0; c < ColorCount; ++c)
                    cell->SetClass(PaletteClass(c), c == color);
            }
            if (auto* cell = ElementById(
                    document_, IndexedId("mcCheckGuildMark-mcBlock", i).c_str()))
            {
                for (std::uint8_t c = 0; c < ColorCount; ++c)
                    cell->SetClass(PaletteClass(c), c == color);
            }
        }

        for (std::size_t i = 0; i < ColorCount; ++i)
        {
            if (auto* color = ElementById(
                    document_, IndexedId("btnColor", i).c_str()))
                color->SetClass(
                    "selected", i == state_.selectedColor);
        }
    }

    void Queue(
        ActionType type,
        std::size_t index = 0,
        std::uint8_t value = 0)
    {
        Action action;
        action.type = type;
        action.index = index;
        action.value = value;
        action.guildName = state_.guildName;
        action.mark = state_.mark;
        pending_ = std::move(action);
    }

    RmlDocumentHost host_;
    Rml::ElementDocument* document_ = nullptr;
    bool hostVisible_ = false;

    Rml::Element* panel_ = nullptr;
    Rml::Element* drag_ = nullptr;
    Rml::Element* title_ = nullptr;
    Rml::Element* intro_ = nullptr;
    Rml::Element* markPage_ = nullptr;
    Rml::Element* confirmPage_ = nullptr;
    Rml::Element* markGrid_ = nullptr;
    Rml::Element* previewGrid_ = nullptr;
    Rml::Element* colorContainer_ = nullptr;
    Rml::Element* checkName_ = nullptr;
    Rml::Element* checkMaster_ = nullptr;
    Rml::ElementFormControlInput* input_ = nullptr;

    RmlMuButton close_;
    RmlMuButton next_;
    RmlMuButton previous_;
    RmlMuButton confirm_;
    std::array<RmlMuButton, ColorCount> colors_{};
    std::array<RmlMuButton, MarkCellCount> markCells_{};
    RmlMuMovablePanel mover_;

    std::array<float, 2> panelSize_{327.0f, 639.0f};
    std::array<float, 2> referenceSize_{640.0f, 480.0f};
    std::array<float, 2> initialPosition_{20.0f, 20.0f};
    State state_;
    std::optional<Action> pending_;
};

RmlGuildCreatePanel::RmlGuildCreatePanel()
    : m_impl(std::make_unique<Impl>()) {}
RmlGuildCreatePanel::~RmlGuildCreatePanel() = default;
RmlGuildCreatePanel::RmlGuildCreatePanel(RmlGuildCreatePanel&&) noexcept = default;
RmlGuildCreatePanel& RmlGuildCreatePanel::operator=(RmlGuildCreatePanel&&) noexcept = default;

bool RmlGuildCreatePanel::Load(bool show) { return m_impl && m_impl->Load(show); }
bool RmlGuildCreatePanel::Show() { return m_impl && m_impl->Show(); }
bool RmlGuildCreatePanel::Hide() { return m_impl && m_impl->Hide(); }
void RmlGuildCreatePanel::Release() { if (m_impl) m_impl->Release(); }
bool RmlGuildCreatePanel::IsLoaded() const noexcept { return m_impl && m_impl->IsLoaded(); }
bool RmlGuildCreatePanel::ApplyState(const State& state) { return m_impl && m_impl->ApplyState(state); }
bool RmlGuildCreatePanel::Update() { return m_impl && m_impl->Update(); }
std::optional<RmlGuildCreatePanel::Action> RmlGuildCreatePanel::ConsumeAction()
{
    return m_impl ? m_impl->ConsumeAction() : std::nullopt;
}
const RmlGuildCreatePanel::State& RmlGuildCreatePanel::GetState() const noexcept
{
    static const State empty;
    return m_impl ? m_impl->GetState() : empty;
}

} // namespace UI::Modern::PC::Guild

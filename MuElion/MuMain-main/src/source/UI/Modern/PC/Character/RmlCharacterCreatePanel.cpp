#include "stdafx.h"
#include "UI/Modern/PC/Character/RmlCharacterCreatePanel.h"

#include "UI/Modern/RmlDocumentHost.h"
#include "UI/Modern/RmlMuButton.h"
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

namespace UI::Modern::PC::Character
{

namespace
{
constexpr const char* kDocumentPath =
    "Data/UI/PC/Character/character_create.rml";

Rml::String PixelValue(float value)
{
    std::array<char, 64> buffer{};
    std::snprintf(buffer.data(), buffer.size(), "%.3fpx",
                  static_cast<double>(value));
    return Rml::String(buffer.data());
}

Rml::String ScaleValue(float value)
{
    std::array<char, 64> buffer{};
    std::snprintf(buffer.data(), buffer.size(), "scale(%.6f)",
                  static_cast<double>(value));
    return Rml::String(buffer.data());
}

Rml::Element* Get(Rml::ElementDocument* doc, const char* id)
{
    return doc ? doc->GetElementById(id) : nullptr;
}
} // namespace

class RmlCharacterCreatePanel::Impl
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
        if (!document_ || !Bind())
        {
            Release();
            return false;
        }

        visible_ = show;
        dirty_ = true;
        return Refresh();
    }

    [[nodiscard]] bool Show()
    {
        if (!host_.IsLoaded() && !Load(false))
            return false;
        visible_ = true;
        state_.visible = true;
        dirty_ = true;
        return host_.Show();
    }

    [[nodiscard]] bool Hide()
    {
        visible_ = false;
        state_.visible = false;
        return host_.Hide();
    }

    void Release()
    {
        for (auto& button : classButtons_)
            button.Unbind();
        ok_.Unbind();
        cancel_.Unbind();

        document_ = nullptr;
        root_ = nullptr;
        preview_ = nullptr;
        description_ = nullptr;
        input_ = nullptr;
        statName_.fill(nullptr);
        statValue_.fill(nullptr);
        classLabel_.fill(nullptr);

        host_.Release();
        pending_.reset();
        dirty_ = false;
    }

    [[nodiscard]] bool IsLoaded() const noexcept
    {
        return host_.IsLoaded() && document_ != nullptr;
    }

    void SetViewport(int width, int height) noexcept
    {
        state_.viewportWidth = std::max(0, width);
        state_.viewportHeight = std::max(0, height);
        dirty_ = true;
    }

    [[nodiscard]] bool ApplyState(State state)
    {
        const int previousWidth = state_.viewportWidth;
        const int previousHeight = state_.viewportHeight;
        state_ = std::move(state);
        if (state_.viewportWidth <= 0)
            state_.viewportWidth = previousWidth;
        if (state_.viewportHeight <= 0)
            state_.viewportHeight = previousHeight;

        state_.selectedClass = std::clamp(
            state_.selectedClass, 0,
            static_cast<int>(ClassCount) - 1);
        state_.statCount = std::clamp(state_.statCount, 0, 5);
        dirty_ = true;
        return !IsLoaded() || Refresh();
    }

    [[nodiscard]] bool Update()
    {
        if (!IsLoaded())
            return false;

        bool changed = false;

        for (std::size_t index = 0; index < classButtons_.size(); ++index)
        {
            changed |= classButtons_[index].Update();
            if (!classButtons_[index].ConsumeClicked())
                continue;

            pending_ = Action{
                ActionType::SelectClass,
                static_cast<int>(index),
                {},
            };
            changed = true;
        }

        changed |= ok_.Update();
        changed |= cancel_.Update();

        if (ok_.ConsumeClicked())
        {
            pending_ = Action{
                ActionType::Create,
                state_.selectedClass,
                input_ ? std::string(input_->GetValue()) : std::string{},
            };
            changed = true;
        }
        if (cancel_.ConsumeClicked())
        {
            pending_ = Action{
                ActionType::Cancel,
                state_.selectedClass,
                {},
            };
            changed = true;
        }

        if (dirty_)
        {
            changed |= Refresh();
            dirty_ = false;
        }

        return changed;
    }

    [[nodiscard]] std::optional<Action> ConsumeAction()
    {
        return std::exchange(pending_, std::nullopt);
    }

private:
    [[nodiscard]] bool Bind()
    {
        root_ = Get(document_, "panel");
        preview_ = Get(document_, "mcCharacterRender");
        description_ = Get(document_, "taExplain-label");
        input_ = dynamic_cast<Rml::ElementFormControlInput*>(
            Get(document_, "tiCharacterName"));

        if (!root_ || !preview_ || !description_ || !input_)
            return false;

        static constexpr std::array<const char*, 5> kStatRoots{
            "mcStrength", "mcDexterity", "mcVitality",
            "mcEnergy", "mcCharisma"
        };
        for (std::size_t i = 0; i < kStatRoots.size(); ++i)
        {
            const std::string base = kStatRoots[i];
            statName_[i] =
                Get(document_, (base + "-tfTitle-label").c_str());
            statValue_[i] =
                Get(document_, (base + "-tfStat-label").c_str());
            if (!statName_[i] || !statValue_[i])
                return false;
        }

        for (std::size_t i = 0; i < ClassCount; ++i)
        {
            const std::string id = "btnClass" + std::to_string(i);
            Rml::Element* element = Get(document_, id.c_str());
            classLabel_[i] =
                Get(document_, (id + "-label").c_str());
            if (!element || !classLabel_[i])
                return false;
            classButtons_[i].Bind(element);
        }

        Rml::Element* okElement = Get(document_, "btnOK");
        Rml::Element* cancelElement = Get(document_, "btnCancel");
        if (!okElement || !cancelElement)
            return false;

        ok_.Bind(okElement);
        cancel_.Bind(cancelElement);
        input_->SetAttribute("maxlength", 10);
        return true;
    }

    [[nodiscard]] bool Refresh()
    {
        if (!IsLoaded())
            return false;

        root_->SetProperty(
            "display", state_.visible ? "block" : "none");

        if (state_.viewportWidth > 0 && state_.viewportHeight > 0)
        {
            constexpr float authoredWidth = 857.517712f;
            constexpr float authoredHeight = 625.0f;
            const float scale = std::min({
                1.0f,
                static_cast<float>(state_.viewportWidth) / authoredWidth,
                static_cast<float>(state_.viewportHeight) / authoredHeight});
            root_->SetProperty("transform", ScaleValue(scale));
        }

        preview_->SetAttribute("data-class", state_.selectedClass);

        for (std::size_t i = 0; i < ClassCount; ++i)
        {
            classButtons_[i].SetEnabled(state_.classEnabled[i]);
            (void)classButtons_[i].Update();

            if (Rml::Element* element =
                    Get(document_,
                        ("btnClass" + std::to_string(i)).c_str()))
            {
                element->SetClass(
                    "selected",
                    static_cast<int>(i) == state_.selectedClass);
            }

            classLabel_[i]->SetInnerRML(state_.classNames[i]);
        }

        for (std::size_t i = 0; i < statName_.size(); ++i)
        {
            const bool show = static_cast<int>(i) < state_.statCount;
            statName_[i]->GetParentNode()->SetProperty(
                "display", show ? "block" : "none");
            statName_[i]->SetInnerRML(state_.statNames[i]);
            statValue_[i]->SetInnerRML(state_.statValues[i]);
        }

        description_->SetInnerRML(state_.description);
        document_->UpdateDocument();
        return true;
    }

    RmlDocumentHost host_;
    Rml::ElementDocument* document_ = nullptr;
    Rml::Element* root_ = nullptr;
    Rml::Element* preview_ = nullptr;
    Rml::Element* description_ = nullptr;
    Rml::ElementFormControlInput* input_ = nullptr;
    std::array<Rml::Element*, 5> statName_{};
    std::array<Rml::Element*, 5> statValue_{};
    std::array<Rml::Element*, ClassCount> classLabel_{};
    std::array<RmlMuButton, ClassCount> classButtons_{};
    RmlMuButton ok_;
    RmlMuButton cancel_;

    State state_;
    bool visible_ = false;
    bool dirty_ = false;
    std::optional<Action> pending_;
};

RmlCharacterCreatePanel::RmlCharacterCreatePanel()
    : m_impl(std::make_unique<Impl>()) {}
RmlCharacterCreatePanel::~RmlCharacterCreatePanel() = default;
RmlCharacterCreatePanel::RmlCharacterCreatePanel(
    RmlCharacterCreatePanel&&) noexcept = default;
RmlCharacterCreatePanel& RmlCharacterCreatePanel::operator=(
    RmlCharacterCreatePanel&&) noexcept = default;

bool RmlCharacterCreatePanel::Load(bool show)
{ return m_impl && m_impl->Load(show); }
bool RmlCharacterCreatePanel::Show()
{ return m_impl && m_impl->Show(); }
bool RmlCharacterCreatePanel::Hide()
{ return m_impl && m_impl->Hide(); }
void RmlCharacterCreatePanel::Release()
{ if (m_impl) m_impl->Release(); }
bool RmlCharacterCreatePanel::IsLoaded() const noexcept
{ return m_impl && m_impl->IsLoaded(); }
void RmlCharacterCreatePanel::SetViewport(int width, int height) noexcept
{ if (m_impl) m_impl->SetViewport(width, height); }
bool RmlCharacterCreatePanel::ApplyState(State state)
{ return m_impl && m_impl->ApplyState(std::move(state)); }
bool RmlCharacterCreatePanel::Update()
{ return m_impl && m_impl->Update(); }
std::optional<RmlCharacterCreatePanel::Action>
RmlCharacterCreatePanel::ConsumeAction()
{ return m_impl ? m_impl->ConsumeAction() : std::nullopt; }

} // namespace UI::Modern::PC::Character

#include "stdafx.h"
#include "UI/Modern/PC/HUD/RmlMasterTreePanel.h"

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

namespace UI::Modern::PC::HUD
{

namespace
{
constexpr const char* kDocumentPath =
    "Data/UI/PC/HUD/master_tree.rml";

std::vector<float> ParseFloatList(
    const RmlUiDesign::Values& values,
    const char* key,
    std::initializer_list<float> fallback)
{
    const auto it = values.find(key);
    if (it == values.end())
        return {fallback.begin(), fallback.end()};

    std::vector<float> result;
    std::istringstream stream(it->second);
    float value = 0.0f;
    while (stream >> value)
        result.push_back(value);

    if (result.empty())
        result.assign(fallback.begin(), fallback.end());
    return result;
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

Rml::Element* ElementById(
    Rml::ElementDocument* document, const Rml::String& id)
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
} // namespace

class RmlMasterTreePanel::Impl
{
public:
    struct Design
    {
        float panelWidth = 925.9f;
        float panelHeight = 711.9f;
        int skillColumns = 25;
        float legacyWidth = 640.0f;
        float legacyHeight = 480.0f;
        float tooltipBottomThreshold = 300.0f;

        // Existing MuMain master-tree geometry recovered from
        // CNewUIMasterLevel.
        std::array<float, CategoryCount> categoryX{
            11.0f, 221.0f, 431.0f};
        float categoryY = 55.0f;
        float columnStep = 49.0f;
        float rankStep = 41.0f;
        float legacySlotWidth = 50.0f;
        float legacySlotHeight = 38.0f;
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
        closeButton_.Unbind();
        expButton_.Unbind();
        for (auto& button : skillButtons_)
            button.Unbind();
        mover_.Unbind();

        root_ = nullptr;
        drag_ = nullptr;
        expElement_ = nullptr;
        closeElement_ = nullptr;
        expText_ = nullptr;
        pointText_ = nullptr;
        levelText_ = nullptr;
        classText_ = nullptr;
        categoryTexts_.fill(nullptr);
        slotElements_.fill(nullptr);
        skillSheetElements_.fill(nullptr);
        rankElements_.fill(nullptr);

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

        ApplyLayout();

        SetText(expText_, state_.experience);
        SetText(pointText_, state_.points);
        SetText(levelText_, state_.level);
        SetText(classText_, state_.characterClass);
        for (std::size_t i = 0; i < CategoryCount; ++i)
            SetText(categoryTexts_[i], state_.categoryLabels[i]);

        for (std::size_t i = 0; i < SlotCount; ++i)
        {
            skillButtons_[i].SetVisible(false);
            skillButtons_[i].SetEnabled(false);
            (void)skillButtons_[i].Update();
            slotElements_[i]->SetProperty("display", "none");
            slotSkillIds_[i] = -1;
        }

        for (const SkillSlot& skill : state_.skills)
        {
            if (!skill.visible ||
                skill.slotId == 0 ||
                skill.slotId > SlotCount)
                continue;

            const std::size_t index = skill.slotId - 1u;
            Rml::Element* slot = slotElements_[index];
            Rml::Element* sheet = skillSheetElements_[index];

            slot->SetProperty("display", "block");
            for (int direction = 0; direction <= 8; ++direction)
            {
                slot->SetClass(
                    Rml::String("direction-") +
                        std::to_string(direction),
                    direction == std::clamp(skill.direction, 0, 8));
            }

            slot->SetClass("available", skill.enabled);
            slot->SetClass(
                "maxed",
                skill.maxLevel > 0 && skill.level >= skill.maxLevel);

            PositionSkill(*slot, skill);
            SetText(rankElements_[index], std::to_string(skill.level));

            Rml::String classes = "skill-sheet";
            if (!skill.iconClass.empty())
                classes += Rml::String(" ") + skill.iconClass;
            sheet->SetClassNames(classes);

            skillButtons_[index].SetVisible(true);
            skillButtons_[index].SetEnabled(skill.enabled);
            (void)skillButtons_[index].Update();
            slotSkillIds_[index] = skill.skillId;
        }

        (void)closeButton_.Update();
        (void)expButton_.Update();
        document_->UpdateDocument();
        return true;
    }

    [[nodiscard]] bool Update()
    {
        if (!IsLoaded())
            return false;

        bool changed = false;

        changed |= closeButton_.Update();
        if (closeButton_.ConsumeClicked())
        {
            pendingAction_ = Action{ActionType::Close, -1};
            changed = true;
        }

        changed |= expButton_.Update();
        if (expButton_.ConsumeClicked())
        {
            pendingAction_ = Action{ActionType::Experience, -1};
            changed = true;
        }

        for (std::size_t i = 0; i < SlotCount; ++i)
        {
            changed |= skillButtons_[i].Update();
            if (!skillButtons_[i].ConsumeClicked())
                continue;

            if (slotSkillIds_[i] >= 0)
            {
                pendingAction_ =
                    Action{ActionType::UpgradeSkill, slotSkillIds_[i]};
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

        const auto panelSize = ParseFloatList(
            values, "Panel-Size", {925.9f, 711.9f});
        if (panelSize.size() >= 2)
        {
            design_.panelWidth = panelSize[0];
            design_.panelHeight = panelSize[1];
        }

        const auto reference = ParseFloatList(
            values, "Legacy-ReferenceSize", {640.0f, 480.0f});
        if (reference.size() >= 2)
        {
            design_.legacyWidth = reference[0];
            design_.legacyHeight = reference[1];
        }

        (void)ParseScalar(
            values, "Skill-Columns", design_.skillColumns);
        (void)ParseScalar(
            values, "Tooltip-BottomThreshold",
            design_.tooltipBottomThreshold);
    }

    [[nodiscard]] bool BindElements()
    {
        root_ = ElementById(document_, "master-tree");
        drag_ = ElementById(document_, "btnDrag");
        expElement_ = ElementById(document_, "btnExp");
        closeElement_ = ElementById(document_, "btnClose");
        expText_ = ElementById(document_, "tfExp");
        pointText_ = ElementById(document_, "tfPoint");
        levelText_ = ElementById(document_, "tfLevel");
        classText_ = ElementById(document_, "tfClass");
        categoryTexts_[0] = ElementById(document_, "tfAttribute1");
        categoryTexts_[1] = ElementById(document_, "tfAttribute2");
        categoryTexts_[2] = ElementById(document_, "tfAttribute3");

        if (!root_ || !drag_ || !expElement_ || !closeElement_ ||
            !expText_ || !pointText_ || !levelText_ || !classText_ ||
            !categoryTexts_[0] || !categoryTexts_[1] ||
            !categoryTexts_[2])
            return false;

        for (std::size_t i = 0; i < SlotCount; ++i)
        {
            const std::string number = std::to_string(i + 1u);
            slotElements_[i] =
                ElementById(document_, "mcSkillSlot" + number);
            Rml::Element* button =
                ElementById(document_, "mcSkillSlot" + number + "-btnBase");
            skillSheetElements_[i] =
                ElementById(document_, "master-skill-sheet-" + number);
            rankElements_[i] =
                ElementById(document_, "master-skill-rank-" + number);

            if (!slotElements_[i] || !button ||
                !skillSheetElements_[i] || !rankElements_[i])
                return false;

            skillButtons_[i].Bind(button);
        }

        closeButton_.Bind(closeElement_);
        expButton_.Bind(expElement_);
        mover_.Bind(root_, drag_);

        const Rml::Vector2f current = mover_.GetPosition();
        if (current.x == 0.0f && current.y == 0.0f)
            mover_.SetPosition(0.0f, 0.0f);
        return true;
    }

    void ApplyLayout()
    {
        const float availableWidth =
            static_cast<float>(state_.viewportWidth);
        const float availableHeight =
            static_cast<float>(state_.viewportHeight);

        const float scale = std::clamp(
            std::min(
                availableWidth / std::max(1.0f, design_.panelWidth),
                availableHeight / std::max(1.0f, design_.panelHeight)),
            0.2f,
            1.0f);

        root_->SetProperty(
            "width", PixelValue(design_.panelWidth));
        root_->SetProperty(
            "height", PixelValue(design_.panelHeight));
        root_->SetProperty("transform-origin", "0 0");
        root_->SetProperty("transform", ScaleValue(scale));

        mover_.SetMetrics(
            availableWidth / scale,
            availableHeight / scale,
            design_.panelWidth,
            design_.panelHeight,
            0.0f,
            0.0f);

        if (state_.applyPosition)
        {
            mover_.SetPosition(
                state_.positionX / scale,
                state_.positionY / scale);
        }
        else
        {
            const Rml::Vector2f current = mover_.GetPosition();
            if (current.x == 0.0f && current.y == 0.0f)
            {
                mover_.SetPosition(
                    std::max(
                        0.0f,
                        (availableWidth / scale -
                         design_.panelWidth) * 0.5f),
                    std::max(
                        0.0f,
                        (availableHeight / scale -
                         design_.panelHeight) * 0.5f));
            }
        }
    }

    void PositionSkill(Rml::Element& slot, const SkillSlot& skill)
    {
        const int group =
            std::clamp(skill.group, 0, int(CategoryCount - 1));
        const int rank = std::max(1, skill.rank);
        const int column = std::clamp(skill.column, 0, 3);

        const float sx =
            design_.panelWidth / std::max(1.0f, design_.legacyWidth);
        const float sy =
            design_.panelHeight / std::max(1.0f, design_.legacyHeight);

        const float x =
            (design_.categoryX[group] +
             design_.columnStep * static_cast<float>(column)) * sx;
        const float y =
            (design_.categoryY +
             design_.rankStep * static_cast<float>(rank - 1)) * sy;

        slot.SetProperty("left", PixelValue(x));
        slot.SetProperty("top", PixelValue(y));
        slot.SetProperty(
            "width", PixelValue(design_.legacySlotWidth * sx));
        slot.SetProperty(
            "height", PixelValue(design_.legacySlotHeight * sy));
    }

    RmlDocumentHost host_;
    Rml::ElementDocument* document_ = nullptr;
    bool hostVisible_ = false;

    Rml::Element* root_ = nullptr;
    Rml::Element* drag_ = nullptr;
    Rml::Element* expElement_ = nullptr;
    Rml::Element* closeElement_ = nullptr;
    Rml::Element* expText_ = nullptr;
    Rml::Element* pointText_ = nullptr;
    Rml::Element* levelText_ = nullptr;
    Rml::Element* classText_ = nullptr;
    std::array<Rml::Element*, CategoryCount> categoryTexts_{};

    std::array<Rml::Element*, SlotCount> slotElements_{};
    std::array<Rml::Element*, SlotCount> skillSheetElements_{};
    std::array<Rml::Element*, SlotCount> rankElements_{};
    std::array<RmlMuButton, SlotCount> skillButtons_{};
    std::array<int, SlotCount> slotSkillIds_{};

    RmlMuButton closeButton_;
    RmlMuButton expButton_;
    RmlMuMovablePanel mover_;

    Design design_;
    State state_;
    std::optional<Action> pendingAction_;
};

RmlMasterTreePanel::RmlMasterTreePanel()
    : m_impl(std::make_unique<Impl>())
{
}
RmlMasterTreePanel::~RmlMasterTreePanel() = default;
RmlMasterTreePanel::RmlMasterTreePanel(
    RmlMasterTreePanel&&) noexcept = default;
RmlMasterTreePanel& RmlMasterTreePanel::operator=(
    RmlMasterTreePanel&&) noexcept = default;

bool RmlMasterTreePanel::Load(bool show)
{
    return m_impl && m_impl->Load(show);
}
bool RmlMasterTreePanel::Show()
{
    return m_impl && m_impl->Show();
}
bool RmlMasterTreePanel::Hide()
{
    return m_impl && m_impl->Hide();
}
void RmlMasterTreePanel::Release()
{
    if (m_impl) m_impl->Release();
}
bool RmlMasterTreePanel::IsLoaded() const noexcept
{
    return m_impl && m_impl->IsLoaded();
}
bool RmlMasterTreePanel::ApplyState(const State& state)
{
    return m_impl && m_impl->ApplyState(state);
}
bool RmlMasterTreePanel::Update()
{
    return m_impl && m_impl->Update();
}
std::optional<RmlMasterTreePanel::Action>
RmlMasterTreePanel::ConsumeAction()
{
    return m_impl ? m_impl->ConsumeAction() : std::nullopt;
}

} // namespace UI::Modern::PC::HUD

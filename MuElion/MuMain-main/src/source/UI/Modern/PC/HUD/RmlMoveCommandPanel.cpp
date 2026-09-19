#include "stdafx.h"
#include "UI/Modern/PC/HUD/RmlMoveCommandPanel.h"

#include "UI/Modern/RmlDocumentHost.h"
#include "UI/Modern/RmlMuButton.h"
#include "UI/Modern/RmlMuScrollBar.h"
#include "UI/Modern/RmlUiDesign.h"

#include <RmlUi/Core/Core.h>
#include <RmlUi/Core/Element.h>
#include <RmlUi/Core/ElementDocument.h>
#include <RmlUi/Core/Event.h>
#include <RmlUi/Core/EventListener.h>
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
    "Data/UI/PC/HUD/move_command.rml";

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

class RmlMoveCommandPanel::Impl final : public Rml::EventListener
{
public:
    struct Design
    {
        float panelWidth = 376.998901f;
        float panelHeight = 532.997894f;
        float mainListX = 27.0f;
        float mainListY = 89.0f;
        float favoriteListY = 362.0f;
        float showMapX = 111.0f;
        float closeX = 192.0f;
        float textButtonY = 484.0f;
        float textButtonWidth = 75.0f;
        float textButtonHeight = 35.0f;
        float scrollBarX = 331.35f;
        float scrollBarY = 82.0f;
        float rowHeight = 20.0f;
        float rowWidth = 295.935713f;
        float checkBoxX = 262.0f;
        float checkBoxWidth = 42.600001f;
        float scrollDownY = 239.00032f;
        float scrollTrackHeight = 231.50032f;
        float rootY = 50.0f;
        float referenceWidth = 640.0f;
        float referenceHeight = 480.0f;
    };

    Impl() : host_(kDocumentPath) {}
    ~Impl() override { Release(); }

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
        for (Rml::Element* element : mainFavoriteElements_)
        {
            if (element)
                element->RemoveEventListener("click", this, false);
        }

        for (auto& button : mainRowButtons_)
            button.Unbind();
        for (auto& button : favoriteRowButtons_)
            button.Unbind();

        showMapButton_.Unbind();
        closeButton_.Unbind();
        scrollBar_.Unbind();

        root_ = nullptr;
        mainList_ = nullptr;
        favoriteList_ = nullptr;
        title_ = nullptr;
        strifeLabel_ = nullptr;
        mapLabel_ = nullptr;
        levelLabel_ = nullptr;
        zenLabel_ = nullptr;
        favoriteLabel_ = nullptr;
        favoriteTitle_ = nullptr;
        showMapLabel_ = nullptr;
        closeLabel_ = nullptr;

        mainRowElements_.fill(nullptr);
        mainFavoriteElements_.fill(nullptr);
        mainStrifeElements_.fill(nullptr);
        mainMapElements_.fill(nullptr);
        mainLevelElements_.fill(nullptr);
        mainZenElements_.fill(nullptr);

        favoriteRowElements_.fill(nullptr);
        favoriteStrifeElements_.fill(nullptr);
        favoriteMapElements_.fill(nullptr);
        favoriteLevelElements_.fill(nullptr);
        favoriteZenElements_.fill(nullptr);

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

        SetText(title_, state_.title);
        SetText(strifeLabel_, state_.strifeLabel);
        SetText(mapLabel_, state_.mapLabel);
        SetText(levelLabel_, state_.levelLabel);
        SetText(zenLabel_, state_.zenLabel);
        SetText(favoriteLabel_, state_.favoriteLabel);
        SetText(favoriteTitle_, state_.favoriteTitle);
        SetText(showMapLabel_, state_.showMapLabel);
        SetText(closeLabel_, state_.closeLabel);

        const std::size_t maximum =
            state_.destinations.size() > MainRowCount
                ? state_.destinations.size() - MainRowCount
                : 0u;
        scrollStart_ = std::min(scrollStart_, maximum);

        for (std::size_t slot = 0; slot < MainRowCount; ++slot)
        {
            const std::size_t absolute = scrollStart_ + slot;
            const bool present = absolute < state_.destinations.size();

            mainRowButtons_[slot].SetVisible(present);
            mainRowButtons_[slot].SetEnabled(
                present && state_.destinations[absolute].canMove);
            (void)mainRowButtons_[slot].Update();

            if (!present)
                continue;

            const Destination& destination =
                state_.destinations[absolute];

            SetText(mainStrifeElements_[slot], destination.strife);
            SetText(mainMapElements_[slot], destination.map);
            SetText(mainLevelElements_[slot], destination.level);
            SetText(mainZenElements_[slot], destination.zen);

            mainRowElements_[slot]->SetClass(
                "selected",
                state_.selectedMapIndex &&
                *state_.selectedMapIndex == destination.mapIndex);
            mainRowElements_[slot]->SetClass(
                "unavailable", !destination.canMove);
            mainFavoriteElements_[slot]->SetClass(
                "checked", destination.favorite);
            mainFavoriteElements_[slot]->SetAttribute(
                "data-map-index", destination.mapIndex);
        }

        for (std::size_t slot = 0; slot < FavoriteRowCount; ++slot)
        {
            const bool present = slot < state_.favorites.size();
            favoriteRowButtons_[slot].SetVisible(present);
            favoriteRowButtons_[slot].SetEnabled(
                present && state_.favorites[slot].canMove);
            (void)favoriteRowButtons_[slot].Update();

            if (!present)
                continue;

            const Destination& destination = state_.favorites[slot];
            SetText(favoriteStrifeElements_[slot], destination.strife);
            SetText(favoriteMapElements_[slot], destination.map);
            SetText(favoriteLevelElements_[slot], destination.level);
            SetText(favoriteZenElements_[slot], destination.zen);
            favoriteRowElements_[slot]->SetClass(
                "selected",
                state_.selectedMapIndex &&
                *state_.selectedMapIndex == destination.mapIndex);
            favoriteRowElements_[slot]->SetClass(
                "unavailable", !destination.canMove);
            favoriteRowElements_[slot]->SetAttribute(
                "data-map-index", destination.mapIndex);
        }

        (void)scrollBar_.SetState(
            scrollStart_,
            maximum,
            MainRowCount,
            MainRowCount,
            maximum > 0,
            true);

        const bool hasSelection = state_.selectedMapIndex.has_value();
        showMapButton_.SetEnabled(hasSelection);
        (void)showMapButton_.Update();
        (void)closeButton_.Update();

        document_->UpdateDocument();
        return true;
    }

    [[nodiscard]] bool Update()
    {
        if (!IsLoaded())
            return false;

        bool changed = false;

        if (const auto requested = scrollBar_.ConsumeRequestedPosition())
        {
            scrollStart_ = *requested;
            (void)ApplyState(state_);
            changed = true;
        }

        for (std::size_t slot = 0; slot < MainRowCount; ++slot)
        {
            changed |= mainRowButtons_[slot].Update();
            if (!mainRowButtons_[slot].ConsumeClicked())
                continue;

            const std::size_t absolute = scrollStart_ + slot;
            if (absolute < state_.destinations.size())
            {
                pendingAction_ = Action{
                    ActionType::SelectDestination,
                    state_.destinations[absolute].mapIndex};
                changed = true;
            }
        }

        for (std::size_t slot = 0; slot < FavoriteRowCount; ++slot)
        {
            changed |= favoriteRowButtons_[slot].Update();
            if (!favoriteRowButtons_[slot].ConsumeClicked())
                continue;

            if (slot < state_.favorites.size())
            {
                pendingAction_ = Action{
                    ActionType::SelectDestination,
                    state_.favorites[slot].mapIndex};
                changed = true;
            }
        }

        changed |= showMapButton_.Update();
        changed |= closeButton_.Update();

        if (showMapButton_.ConsumeClicked())
        {
            pendingAction_ = Action{
                ActionType::ShowMap,
                state_.selectedMapIndex.value_or(-1)};
            changed = true;
        }

        if (closeButton_.ConsumeClicked())
        {
            pendingAction_ = Action{ActionType::Close, -1};
            changed = true;
        }

        return changed;
    }

    [[nodiscard]] std::optional<Action> ConsumeAction()
    {
        return std::exchange(pendingAction_, std::nullopt);
    }

    void ProcessEvent(Rml::Event& event) override
    {
        if (event.GetType() != "click")
            return;

        Rml::Element* element = event.GetCurrentElement();
        if (!element)
            return;

        const int mapIndex =
            element->GetAttribute<int>("data-map-index", -1);
        if (mapIndex < 0)
            return;

        pendingAction_ =
            Action{ActionType::ToggleFavorite, mapIndex};
        event.StopPropagation();
    }

private:
    void ReadDesign()
    {
        const RmlUiDesign::Values values = LoadDesignValues();

        (void)ParseScalar(values, "MoveCommand-PanelWidth", design_.panelWidth);
        (void)ParseScalar(values, "MoveCommand-PanelHeight", design_.panelHeight);
        (void)ParseScalar(values, "MoveCommand-MainListX", design_.mainListX);
        (void)ParseScalar(values, "MoveCommand-MainListY", design_.mainListY);
        (void)ParseScalar(values, "MoveCommand-FavoriteListY", design_.favoriteListY);
        (void)ParseScalar(values, "MoveCommand-ShowMapX", design_.showMapX);
        (void)ParseScalar(values, "MoveCommand-CloseX", design_.closeX);
        (void)ParseScalar(values, "MoveCommand-TextButtonY", design_.textButtonY);
        (void)ParseScalar(values, "MoveCommand-TextButtonWidth", design_.textButtonWidth);
        (void)ParseScalar(values, "MoveCommand-TextButtonHeight", design_.textButtonHeight);
        (void)ParseScalar(values, "MoveCommand-ScrollBarX", design_.scrollBarX);
        (void)ParseScalar(values, "MoveCommand-ScrollBarY", design_.scrollBarY);
        (void)ParseScalar(values, "MoveCommand-RowHeight", design_.rowHeight);
        (void)ParseScalar(values, "MoveCommand-RowWidth", design_.rowWidth);
        (void)ParseScalar(values, "MoveCommand-CheckBoxX", design_.checkBoxX);
        (void)ParseScalar(values, "MoveCommand-CheckBoxWidth", design_.checkBoxWidth);
        (void)ParseScalar(values, "MoveCommand-ScrollDownY", design_.scrollDownY);
        (void)ParseScalar(values, "MoveCommand-ScrollTrackHeight", design_.scrollTrackHeight);
        (void)ParseScalar(values, "MoveCommand-RootY", design_.rootY);
        (void)ParseScalar(values, "MoveCommand-ReferenceWidth", design_.referenceWidth);
        (void)ParseScalar(values, "MoveCommand-ReferenceHeight", design_.referenceHeight);
    }

    [[nodiscard]] bool BindElements()
    {
        root_ = ElementById(document_, "move-command");
        mainList_ = ElementById(document_, "move-command-main-list");
        favoriteList_ = ElementById(document_, "move-command-favorite-list");
        title_ = ElementById(document_, "move-command-title");
        strifeLabel_ = ElementById(document_, "move-command-strife-label");
        mapLabel_ = ElementById(document_, "move-command-map-label");
        levelLabel_ = ElementById(document_, "move-command-level-label");
        zenLabel_ = ElementById(document_, "move-command-zen-label");
        favoriteLabel_ = ElementById(document_, "move-command-favorite-label");
        favoriteTitle_ = ElementById(document_, "move-command-favorite-title");
        showMapLabel_ = ElementById(document_, "move-command-show-map-label");
        closeLabel_ = ElementById(document_, "move-command-close-label");

        if (!root_ || !mainList_ || !favoriteList_ || !title_ ||
            !strifeLabel_ || !mapLabel_ || !levelLabel_ || !zenLabel_ ||
            !favoriteLabel_ || !favoriteTitle_ ||
            !showMapLabel_ || !closeLabel_)
            return false;

        for (std::size_t i = 0; i < MainRowCount; ++i)
        {
            const std::string suffix = std::to_string(i);
            mainRowElements_[i] =
                ElementById(document_, "move-command-main-row-" + suffix);
            mainFavoriteElements_[i] =
                ElementById(document_, "move-command-main-row-art-" + suffix);
            mainStrifeElements_[i] =
                ElementById(document_, "move-command-main-row-strife-" + suffix);
            mainMapElements_[i] =
                ElementById(document_, "move-command-main-row-map-" + suffix);
            mainLevelElements_[i] =
                ElementById(document_, "move-command-main-row-level-" + suffix);
            mainZenElements_[i] =
                ElementById(document_, "move-command-main-row-zen-" + suffix);

            if (!mainRowElements_[i] || !mainFavoriteElements_[i] ||
                !mainStrifeElements_[i] || !mainMapElements_[i] ||
                !mainLevelElements_[i] || !mainZenElements_[i])
                return false;

            mainRowButtons_[i].Bind(mainRowElements_[i]);
            mainFavoriteElements_[i]->AddEventListener(
                "click", this, false);
        }

        for (std::size_t i = 0; i < FavoriteRowCount; ++i)
        {
            const std::string suffix = std::to_string(i);
            favoriteRowElements_[i] =
                ElementById(document_, "move-command-favorite-row-" + suffix);
            favoriteStrifeElements_[i] =
                ElementById(document_, "move-command-favorite-row-strife-" + suffix);
            favoriteMapElements_[i] =
                ElementById(document_, "move-command-favorite-row-map-" + suffix);
            favoriteLevelElements_[i] =
                ElementById(document_, "move-command-favorite-row-level-" + suffix);
            favoriteZenElements_[i] =
                ElementById(document_, "move-command-favorite-row-zen-" + suffix);

            if (!favoriteRowElements_[i] ||
                !favoriteStrifeElements_[i] || !favoriteMapElements_[i] ||
                !favoriteLevelElements_[i] || !favoriteZenElements_[i])
                return false;

            favoriteRowButtons_[i].Bind(favoriteRowElements_[i]);
        }

        if (!scrollBar_.Bind(
                ElementById(document_, "move-command-scroll")))
            return false;

        showMapButton_.Bind(
            ElementById(document_, "move-command-show-map"));
        closeButton_.Bind(
            ElementById(document_, "move-command-close"));
        return true;
    }

    void ApplyLayout()
    {
        const float width = std::max(1.0f, design_.panelWidth);
        const float height = std::max(1.0f, design_.panelHeight);

        const float availableWidth =
            static_cast<float>(state_.viewportWidth);
        const float availableHeight =
            static_cast<float>(state_.viewportHeight);

        const float scale = std::clamp(
            std::min(
                availableWidth / width,
                availableHeight /
                    std::max(1.0f, design_.rootY + height)),
            0.25f,
            1.0f);

        root_->SetProperty("width", PixelValue(width));
        root_->SetProperty("height", PixelValue(height));
        root_->SetProperty("transform-origin", "0 0");
        root_->SetProperty("transform", ScaleValue(scale));
        root_->SetProperty(
            "left",
            PixelValue(std::max(
                0.0f,
                (availableWidth - width * scale) * 0.5f)));
        root_->SetProperty(
            "top", PixelValue(design_.rootY * scale));

        mainList_->SetProperty("left", PixelValue(design_.mainListX));
        mainList_->SetProperty("top", PixelValue(design_.mainListY));
        mainList_->SetProperty("width", PixelValue(design_.rowWidth));
        mainList_->SetProperty(
            "height",
            PixelValue(design_.rowHeight * MainRowCount));

        favoriteList_->SetProperty("left", PixelValue(design_.mainListX));
        favoriteList_->SetProperty("top", PixelValue(design_.favoriteListY));
        favoriteList_->SetProperty("width", PixelValue(design_.rowWidth));
        favoriteList_->SetProperty(
            "height",
            PixelValue(design_.rowHeight * FavoriteRowCount));

        for (std::size_t i = 0; i < MainRowCount; ++i)
        {
            mainRowElements_[i]->SetProperty(
                "height", PixelValue(design_.rowHeight));
            mainFavoriteElements_[i]->SetProperty(
                "left", PixelValue(design_.checkBoxX));
            mainFavoriteElements_[i]->SetProperty(
                "width", PixelValue(design_.checkBoxWidth));
        }

        for (std::size_t i = 0; i < FavoriteRowCount; ++i)
        {
            favoriteRowElements_[i]->SetProperty(
                "height", PixelValue(design_.rowHeight));
        }

        Rml::Element* scroll =
            ElementById(document_, "move-command-scroll");
        scroll->SetProperty("left", PixelValue(design_.scrollBarX));
        scroll->SetProperty("top", PixelValue(design_.scrollBarY));

        Rml::Element* showMap =
            ElementById(document_, "move-command-show-map");
        Rml::Element* close =
            ElementById(document_, "move-command-close");

        showMap->SetProperty("left", PixelValue(design_.showMapX));
        showMap->SetProperty("top", PixelValue(design_.textButtonY));
        showMap->SetProperty("width", PixelValue(design_.textButtonWidth));
        showMap->SetProperty("height", PixelValue(design_.textButtonHeight));

        close->SetProperty("left", PixelValue(design_.closeX));
        close->SetProperty("top", PixelValue(design_.textButtonY));
        close->SetProperty("width", PixelValue(design_.textButtonWidth));
        close->SetProperty("height", PixelValue(design_.textButtonHeight));
    }

    RmlDocumentHost host_;
    Rml::ElementDocument* document_ = nullptr;
    bool hostVisible_ = false;

    Rml::Element* root_ = nullptr;
    Rml::Element* mainList_ = nullptr;
    Rml::Element* favoriteList_ = nullptr;
    Rml::Element* title_ = nullptr;
    Rml::Element* strifeLabel_ = nullptr;
    Rml::Element* mapLabel_ = nullptr;
    Rml::Element* levelLabel_ = nullptr;
    Rml::Element* zenLabel_ = nullptr;
    Rml::Element* favoriteLabel_ = nullptr;
    Rml::Element* favoriteTitle_ = nullptr;
    Rml::Element* showMapLabel_ = nullptr;
    Rml::Element* closeLabel_ = nullptr;

    std::array<Rml::Element*, MainRowCount> mainRowElements_{};
    std::array<Rml::Element*, MainRowCount> mainFavoriteElements_{};
    std::array<Rml::Element*, MainRowCount> mainStrifeElements_{};
    std::array<Rml::Element*, MainRowCount> mainMapElements_{};
    std::array<Rml::Element*, MainRowCount> mainLevelElements_{};
    std::array<Rml::Element*, MainRowCount> mainZenElements_{};

    std::array<Rml::Element*, FavoriteRowCount> favoriteRowElements_{};
    std::array<Rml::Element*, FavoriteRowCount> favoriteStrifeElements_{};
    std::array<Rml::Element*, FavoriteRowCount> favoriteMapElements_{};
    std::array<Rml::Element*, FavoriteRowCount> favoriteLevelElements_{};
    std::array<Rml::Element*, FavoriteRowCount> favoriteZenElements_{};

    std::array<RmlMuButton, MainRowCount> mainRowButtons_{};
    std::array<RmlMuButton, FavoriteRowCount> favoriteRowButtons_{};
    RmlMuScrollBar scrollBar_;
    RmlMuButton showMapButton_;
    RmlMuButton closeButton_;

    Design design_;
    State state_;
    std::size_t scrollStart_ = 0;
    std::optional<Action> pendingAction_;
};

RmlMoveCommandPanel::RmlMoveCommandPanel()
    : m_impl(std::make_unique<Impl>()) {}
RmlMoveCommandPanel::~RmlMoveCommandPanel() = default;
RmlMoveCommandPanel::RmlMoveCommandPanel(
    RmlMoveCommandPanel&&) noexcept = default;
RmlMoveCommandPanel& RmlMoveCommandPanel::operator=(
    RmlMoveCommandPanel&&) noexcept = default;

bool RmlMoveCommandPanel::Load(bool show)
{
    return m_impl && m_impl->Load(show);
}
bool RmlMoveCommandPanel::Show()
{
    return m_impl && m_impl->Show();
}
bool RmlMoveCommandPanel::Hide()
{
    return m_impl && m_impl->Hide();
}
void RmlMoveCommandPanel::Release()
{
    if (m_impl) m_impl->Release();
}
bool RmlMoveCommandPanel::IsLoaded() const noexcept
{
    return m_impl && m_impl->IsLoaded();
}
bool RmlMoveCommandPanel::ApplyState(const State& state)
{
    return m_impl && m_impl->ApplyState(state);
}
bool RmlMoveCommandPanel::Update()
{
    return m_impl && m_impl->Update();
}
std::optional<RmlMoveCommandPanel::Action>
RmlMoveCommandPanel::ConsumeAction()
{
    return m_impl ? m_impl->ConsumeAction() : std::nullopt;
}

} // namespace UI::Modern::PC::HUD

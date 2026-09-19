#include "stdafx.h"
#include "UI/Modern/PC/Chat/RmlChatPanel.h"

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
#include <RmlUi/Core/Elements/ElementFormControlInput.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <sstream>
#include <span>
#include <utility>

namespace UI::Modern::PC::Chat
{

namespace
{
constexpr const char* kDocumentPath = "Data/UI/PC/Chat/chat.rml";

Rml::String PixelValue(float value)
{
    std::array<char, 64> buffer{};
    std::snprintf(buffer.data(), buffer.size(), "%.3fpx",
                  static_cast<double>(value));
    return Rml::String(buffer.data());
}

template <typename T>
bool ParseScalar(const RmlUiDesign::Values& values,
                 const char* key, T& output)
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

std::vector<float> ParseFloatList(
    const RmlUiDesign::Values& values,
    const char* key,
    std::span<const float> fallback)
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
    Rml::ElementDocument* document, const char* id)
{
    return document ? document->GetElementById(id) : nullptr;
}

Rml::ElementFormControlInput* InputElement(
    Rml::ElementDocument* document, const char* id)
{
    return dynamic_cast<Rml::ElementFormControlInput*>(
        RequiredElement(document, id));
}

} // namespace

class RmlChatPanel::Impl
{
public:
    struct Design
    {
        std::vector<float> viewHeights{91.0f, 192.0f, 308.0f, 427.0f};
        float resizeViewportHeight = 1120.0f;
        float smallStageScale = 0.83f;
        float largeStageBottom = 238.0f;
        float smallStageBottom = 197.0f;
        float viewWidth = 420.0f;
        float rowHeight = 18.0f;
        float editingExtraHeight = 39.0f;
        float viewHeightMargin = 24.0f;
        float menuTop = 77.0f;
        float inputTop = 102.0f;
        float scrollDownBottom = 36.0f;
        float marqueeSecondsPerPixel = 0.08f;
        float shadowOffsetX = 1.0f;
        int alphaStep = 20;
        int alphaMaximum = 90;
        int alphaMinimum = 20;
        int defaultAlpha = 60;
        float blockedWidth = 327.0f;
        float blockedHeight = 387.0f;
        std::size_t blockedVisibleRows = 6;
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
        if (!BindElements())
        {
            Release();
            return false;
        }

        visible_ = show;
        ApplyLayout();
        RefreshMessages(true);
        RefreshBlockedRows(true);
        SyncControlClasses();
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
        UnbindElements();
        document_ = nullptr;
        host_.Release();
    }

    [[nodiscard]] bool IsLoaded() const noexcept
    {
        return host_.IsLoaded() && document_ != nullptr;
    }

    void SetVisible(bool visible)
    {
        if (visible)
            (void)Show();
        else if (host_.IsLoaded())
            (void)Hide();
    }

    void SetEditing(bool editing)
    {
        if (editing_ == editing)
            return;
        editing_ = editing;
        ApplyLayout();
    }

    void SetViewportHeight(float viewportHeight)
    {
        viewportHeight_ = std::max(0.0f, viewportHeight);
        ApplyLayout();
    }

    void ClearMessages()
    {
        messages_.clear();
        scrollStart_ = 0;
        messagesDirty_ = true;
    }

    void AddMessage(Message message)
    {
        const std::size_t oldMaximum = MessageMaximumStart();
        const bool followBottom = scrollStart_ >= oldMaximum;

        messages_.push_back(std::move(message));
        if (followBottom)
            scrollStart_ = MessageMaximumStart();
        messagesDirty_ = true;
    }

    void SetMessages(std::vector<Message> messages)
    {
        messages_ = std::move(messages);
        scrollStart_ = MessageMaximumStart();
        messagesDirty_ = true;
    }

    void SetBlockedNames(std::vector<std::string> names)
    {
        blockedNames_ = std::move(names);
        selectedBlocked_.reset();
        blockedScrollStart_ = std::min(
            blockedScrollStart_, BlockedMaximumStart());
        blockedDirty_ = true;
    }

    [[nodiscard]] const std::vector<std::string>& GetBlockedNames() const noexcept
    {
        return blockedNames_;
    }

    [[nodiscard]] bool Update()
    {
        if (!IsLoaded())
            return false;

        bool changed = false;

        changed |= PollButtons();

        const std::size_t visibleRows = VisibleMessageRows();
        const std::size_t maximum = MessageMaximumStart();
        (void)messageScroll_.SetState(
            scrollStart_, maximum, visibleRows, visibleRows,
            maximum > 0, true);
        if (const auto request = messageScroll_.ConsumeRequestedPosition())
        {
            scrollStart_ = std::min(*request, maximum);
            messagesDirty_ = true;
            changed = true;
        }

        const std::size_t blockedMaximum = BlockedMaximumStart();
        (void)blockedScroll_.SetState(
            blockedScrollStart_, blockedMaximum,
            design_.blockedVisibleRows, design_.blockedVisibleRows,
            blockedMaximum > 0, true);
        if (const auto request = blockedScroll_.ConsumeRequestedPosition())
        {
            blockedScrollStart_ = std::min(*request, blockedMaximum);
            blockedDirty_ = true;
            changed = true;
        }

        for (std::size_t i = 0; i < blockedRowButtons_.size(); ++i)
        {
            if (!blockedRowButtons_[i] ||
                !blockedRowButtons_[i]->ConsumeClicked())
                continue;

            const std::size_t absolute = blockedScrollStart_ + i;
            if (absolute < blockedNames_.size())
            {
                selectedBlocked_ = absolute;
                blockedDirty_ = true;
                changed = true;
            }
        }

        if (messagesDirty_)
        {
            RefreshMessages(false);
            changed = true;
        }
        if (blockedDirty_)
        {
            RefreshBlockedRows(false);
            changed = true;
        }

        changed |= SyncControlClasses();
        return changed;
    }

    [[nodiscard]] OutgoingMode GetOutgoingMode() const noexcept
    {
        return outgoingMode_;
    }

    [[nodiscard]] bool IsWhisperFilterEnabled() const noexcept
    {
        return whisperFilter_;
    }

    [[nodiscard]] bool IsSystemFilterEnabled() const noexcept
    {
        return systemFilter_;
    }

    [[nodiscard]] bool IsWhisperBlocked() const noexcept
    {
        return blockWhisper_;
    }

    [[nodiscard]] int GetAlphaPercent() const noexcept
    {
        return alphaPercent_;
    }

    [[nodiscard]] std::size_t GetSizeIndex() const noexcept
    {
        return sizeIndex_;
    }

    [[nodiscard]] std::optional<std::string> ConsumeBlockRegisterRequest()
    {
        return std::exchange(blockRegisterRequest_, std::nullopt);
    }

    [[nodiscard]] std::optional<std::string> ConsumeBlockDeleteRequest()
    {
        return std::exchange(blockDeleteRequest_, std::nullopt);
    }

private:
    void ReadDesign()
    {
        const RmlUiDesign::Values values = LoadDesignValues();
        const std::array<float, 4> fallbackHeights{91, 192, 308, 427};
        design_.viewHeights = ParseFloatList(
            values, "RmlChatPanel-ViewHeights", fallbackHeights);

        (void)ParseScalar(values, "RmlChatPanel-ResizeViewportHeight", design_.resizeViewportHeight);
        (void)ParseScalar(values, "RmlChatPanel-SmallStageScale", design_.smallStageScale);
        (void)ParseScalar(values, "RmlChatPanel-LargeStageBottom", design_.largeStageBottom);
        (void)ParseScalar(values, "RmlChatPanel-SmallStageBottom", design_.smallStageBottom);
        (void)ParseScalar(values, "RmlChatPanel-ViewWidth", design_.viewWidth);
        (void)ParseScalar(values, "RmlChatPanel-RowHeight", design_.rowHeight);
        (void)ParseScalar(values, "RmlChatPanel-EditingExtraHeight", design_.editingExtraHeight);
        (void)ParseScalar(values, "RmlChatPanel-ViewHeightMargin", design_.viewHeightMargin);
        (void)ParseScalar(values, "RmlChatPanel-MenuTop", design_.menuTop);
        (void)ParseScalar(values, "RmlChatPanel-InputTop", design_.inputTop);
        (void)ParseScalar(values, "RmlChatPanel-ScrollDownBottom", design_.scrollDownBottom);
        (void)ParseScalar(values, "RmlChatPanel-MarqueeSecondsPerPixel", design_.marqueeSecondsPerPixel);
        (void)ParseScalar(values, "RmlChatPanel-ShadowOffsetX", design_.shadowOffsetX);
        (void)ParseScalar(values, "RmlChatPanel-AlphaStep", design_.alphaStep);
        (void)ParseScalar(values, "RmlChatPanel-AlphaMaximum", design_.alphaMaximum);
        (void)ParseScalar(values, "RmlChatPanel-AlphaMinimum", design_.alphaMinimum);
        (void)ParseScalar(values, "RmlChatPanel-DefaultAlpha", design_.defaultAlpha);
        (void)ParseScalar(values, "RmlChatPanel-RmlBlockedChatWidth", design_.blockedWidth);
        (void)ParseScalar(values, "RmlChatPanel-RmlBlockedChatHeight", design_.blockedHeight);
        (void)ParseScalar(values, "RmlChatPanel-RmlBlockedChatVisibleRows", design_.blockedVisibleRows);

        if (design_.viewHeights.empty())
            design_.viewHeights.assign(fallbackHeights.begin(), fallbackHeights.end());
        sizeIndex_ = std::min(sizeIndex_, design_.viewHeights.size() - 1);
        alphaPercent_ = std::clamp(
            design_.defaultAlpha, design_.alphaMinimum, design_.alphaMaximum);
    }

    [[nodiscard]] bool BindElements()
    {
        panel_ = RequiredElement(document_, "chat-panel");
        view_ = RequiredElement(document_, "chat-view");
        background_ = RequiredElement(document_, "chat-background");
        title_ = RequiredElement(document_, "chat-title");
        messagesElement_ = RequiredElement(document_, "chat-messages");
        menu_ = RequiredElement(document_, "chat-menu");
        menuButtons_ = RequiredElement(document_, "chat-menu-buttons");
        inputArea_ = RequiredElement(document_, "chat-input-area");
        mainInput_ = InputElement(document_, "chat-input");
        whisperInput_ = InputElement(document_, "chat-whisper");
        scrollDownElement_ = RequiredElement(document_, "chat-scroll-down");

        blockedPanel_ = RequiredElement(document_, "blocked-chat");
        blockedDrag_ = RequiredElement(document_, "blocked-chat-drag");
        blockedList_ = RequiredElement(document_, "blocked-chat-list");
        blockedInput_ = InputElement(document_, "blocked-chat-input");

        if (!panel_ || !view_ || !background_ || !title_ ||
            !messagesElement_ || !menu_ || !menuButtons_ || !inputArea_ ||
            !mainInput_ || !whisperInput_ || !scrollDownElement_ ||
            !blockedPanel_ || !blockedDrag_ || !blockedList_ || !blockedInput_)
            return false;

        if (!messageScroll_.Bind(RequiredElement(document_, "chat-scrollbar")))
            return false;
        if (!blockedScroll_.Bind(RequiredElement(document_, "blocked-chat-scrollbar")))
            return false;

        blockedMover_.Bind(blockedPanel_, blockedDrag_);

        auto bindButton = [&](RmlMuButton& button, const char* id) {
            Rml::Element* element = RequiredElement(document_, id);
            if (!element)
                return false;
            button.Bind(element);
            return true;
        };

        return bindButton(configButton_, "chat-config") &&
               bindButton(normalButton_, "type-normal") &&
               bindButton(partyButton_, "type-party") &&
               bindButton(guildButton_, "type-guild") &&
               bindButton(gensButton_, "type-gens") &&
               bindButton(whisperFilterButton_, "filter-whisper") &&
               bindButton(systemFilterButton_, "filter-system") &&
               bindButton(blockWhisperButton_, "block-whisper") &&
               bindButton(viewModeButton_, "chat-view-mode") &&
               bindButton(sizeButton_, "chat-size") &&
               bindButton(alphaButton_, "chat-alpha") &&
               bindButton(scrollDownButton_, "chat-scroll-down") &&
               bindButton(blockRegisterButton_, "blocked-chat-register") &&
               bindButton(blockDeleteButton_, "blocked-chat-delete") &&
               bindButton(blockCloseButton_, "blocked-chat-close");
    }

    void UnbindElements()
    {
        blockedRowButtons_.clear();
        messageScroll_.Unbind();
        blockedScroll_.Unbind();
        blockedMover_.Unbind();

        for (RmlMuButton* button : AllButtons())
            button->Unbind();

        panel_ = view_ = background_ = title_ = messagesElement_ = nullptr;
        menu_ = menuButtons_ = inputArea_ = scrollDownElement_ = nullptr;
        blockedPanel_ = blockedDrag_ = blockedList_ = nullptr;
        mainInput_ = whisperInput_ = blockedInput_ = nullptr;
    }

    [[nodiscard]] std::array<RmlMuButton*, 15> AllButtons()
    {
        return {
            &configButton_, &normalButton_, &partyButton_, &guildButton_,
            &gensButton_, &whisperFilterButton_, &systemFilterButton_,
            &blockWhisperButton_, &viewModeButton_, &sizeButton_,
            &alphaButton_, &scrollDownButton_, &blockRegisterButton_,
            &blockDeleteButton_, &blockCloseButton_
        };
    }

    [[nodiscard]] bool PollButtons()
    {
        bool changed = false;
        for (RmlMuButton* button : AllButtons())
            changed |= button->Update();

        auto selectMode = [&](RmlMuButton& button, OutgoingMode mode) {
            if (button.ConsumeClicked())
            {
                outgoingMode_ = mode;
                changed = true;
            }
        };
        selectMode(normalButton_, OutgoingMode::Normal);
        selectMode(partyButton_, OutgoingMode::Party);
        selectMode(guildButton_, OutgoingMode::Guild);
        selectMode(gensButton_, OutgoingMode::Gens);

        if (whisperFilterButton_.ConsumeClicked())
        {
            whisperFilter_ = !whisperFilter_;
            messagesDirty_ = true;
            changed = true;
        }
        if (systemFilterButton_.ConsumeClicked())
        {
            systemFilter_ = !systemFilter_;
            messagesDirty_ = true;
            changed = true;
        }
        if (blockWhisperButton_.ConsumeClicked())
        {
            blockWhisper_ = !blockWhisper_;
            changed = true;
        }
        if (configButton_.ConsumeClicked())
        {
            blockedPanelVisible_ = !blockedPanelVisible_;
            changed = true;
        }
        if (blockCloseButton_.ConsumeClicked())
        {
            blockedPanelVisible_ = false;
            changed = true;
        }
        if (viewModeButton_.ConsumeClicked())
        {
            frameVisible_ = !frameVisible_;
            changed = true;
        }
        if (sizeButton_.ConsumeClicked())
        {
            sizeIndex_ = (sizeIndex_ + 1u) % design_.viewHeights.size();
            ApplyLayout();
            messagesDirty_ = true;
            changed = true;
        }
        if (alphaButton_.ConsumeClicked())
        {
            alphaPercent_ += design_.alphaStep;
            if (alphaPercent_ > design_.alphaMaximum)
                alphaPercent_ = design_.alphaMinimum;
            ApplyAlpha();
            changed = true;
        }
        if (scrollDownButton_.ConsumeClicked())
        {
            scrollStart_ = MessageMaximumStart();
            messagesDirty_ = true;
            changed = true;
        }
        if (blockRegisterButton_.ConsumeClicked() && blockedInput_)
        {
            const std::string value = blockedInput_->GetValue();
            if (!value.empty())
            {
                blockRegisterRequest_ = value;
                blockedInput_->SetValue("");
                changed = true;
            }
        }
        if (blockDeleteButton_.ConsumeClicked() &&
            selectedBlocked_ && *selectedBlocked_ < blockedNames_.size())
        {
            blockDeleteRequest_ = blockedNames_[*selectedBlocked_];
            changed = true;
        }

        return changed;
    }

    [[nodiscard]] bool SyncControlClasses()
    {
        if (!document_)
            return false;

        bool changed = false;
        const auto setSelected = [&](const char* id, bool selected) {
            if (Rml::Element* element = RequiredElement(document_, id))
                element->SetClass("selected", selected);
        };

        setSelected("type-normal", outgoingMode_ == OutgoingMode::Normal);
        setSelected("type-party", outgoingMode_ == OutgoingMode::Party);
        setSelected("type-guild", outgoingMode_ == OutgoingMode::Guild);
        setSelected("type-gens", outgoingMode_ == OutgoingMode::Gens);
        setSelected("filter-whisper", whisperFilter_);
        setSelected("filter-system", systemFilter_);
        setSelected("block-whisper", blockWhisper_);

        if (background_)
            background_->SetProperty("display", frameVisible_ ? "block" : "none");
        if (title_)
            title_->SetProperty("display", frameVisible_ ? "block" : "none");

        if (blockedPanel_)
            blockedPanel_->SetProperty(
                "display", blockedPanelVisible_ ? "block" : "none");

        if (scrollDownElement_)
        {
            const bool showDown = scrollStart_ < MessageMaximumStart();
            scrollDownElement_->SetProperty(
                "display", showDown ? "block" : "none");
        }

        changed |= blockedMover_.ConsumePositionChanged();
        return changed;
    }

    void ApplyLayout()
    {
        if (!panel_ || !view_ || !messagesElement_ || !menu_ || !inputArea_)
            return;

        sizeIndex_ = std::min(sizeIndex_, design_.viewHeights.size() - 1);
        const float viewHeight = design_.viewHeights[sizeIndex_];
        const float baseViewHeight = design_.viewHeights.front();

        panel_->SetProperty("width", PixelValue(design_.viewWidth));
        view_->SetProperty("width", PixelValue(design_.viewWidth));
        view_->SetProperty("height", PixelValue(viewHeight));

        messagesElement_->SetProperty(
            "height", PixelValue(std::max(
                design_.rowHeight,
                viewHeight - design_.viewHeightMargin)));

        const float menuOffset = baseViewHeight - design_.menuTop;
        const float inputOffset = design_.inputTop - baseViewHeight;
        menu_->SetProperty("top", PixelValue(viewHeight - menuOffset));
        inputArea_->SetProperty("top", PixelValue(viewHeight + inputOffset));
        inputArea_->SetProperty("display", editing_ ? "block" : "none");

        if (scrollDownElement_)
            scrollDownElement_->SetProperty(
                "bottom", PixelValue(design_.scrollDownBottom));

        if (blockedPanel_)
        {
            blockedPanel_->SetProperty("width", PixelValue(design_.blockedWidth));
            blockedPanel_->SetProperty("height", PixelValue(design_.blockedHeight));
        }

        if (document_)
            document_->UpdateDocument();

        ApplyAlpha();
        UpdateBlockedMoverMetrics();
    }

    void ApplyAlpha()
    {
        if (!background_)
            return;

        const float alpha = static_cast<float>(alphaPercent_) / 100.0f;
        std::array<char, 32> buffer{};
        std::snprintf(buffer.data(), buffer.size(), "%.3f",
                      static_cast<double>(alpha));
        background_->SetProperty("opacity", buffer.data());
    }

    void UpdateBlockedMoverMetrics()
    {
        if (!blockedPanel_ || !document_)
            return;

        const Rml::Vector2i dimensions =
            document_->GetContext() ? document_->GetContext()->GetDimensions()
                                   : Rml::Vector2i{};
        const Rml::Vector2f offset =
            blockedPanel_->GetAbsoluteOffset(Rml::BoxArea::Border);

        blockedMover_.SetMetrics(
            static_cast<float>(dimensions.x),
            static_cast<float>(dimensions.y),
            design_.blockedWidth,
            design_.blockedHeight,
            offset.x,
            offset.y);
    }

    [[nodiscard]] std::size_t VisibleMessageRows() const noexcept
    {
        if (design_.viewHeights.empty() || design_.rowHeight <= 0.0f)
            return 1;

        const float height = std::max(
            design_.rowHeight,
            design_.viewHeights[sizeIndex_] - design_.viewHeightMargin);
        return std::max<std::size_t>(
            1u, static_cast<std::size_t>(height / design_.rowHeight));
    }

    [[nodiscard]] bool MessageVisible(const Message& message) const noexcept
    {
        if (message.kind == MessageKind::Whisper && !whisperFilter_)
            return false;
        if ((message.kind == MessageKind::System ||
             message.kind == MessageKind::Error) && !systemFilter_)
            return false;
        return true;
    }

    [[nodiscard]] std::vector<const Message*> FilteredMessages() const
    {
        std::vector<const Message*> result;
        result.reserve(messages_.size());
        for (const Message& message : messages_)
            if (MessageVisible(message))
                result.push_back(&message);
        return result;
    }

    [[nodiscard]] std::size_t MessageMaximumStart() const
    {
        const auto filtered = FilteredMessages();
        const std::size_t rows = VisibleMessageRows();
        return filtered.size() > rows ? filtered.size() - rows : 0u;
    }

    void RefreshMessages(bool force)
    {
        if (!messagesElement_ || (!messagesDirty_ && !force))
            return;

        messagesDirty_ = false;
        messagesElement_->SetInnerRML("");

        const auto filtered = FilteredMessages();
        const std::size_t rows = VisibleMessageRows();
        const std::size_t maximum =
            filtered.size() > rows ? filtered.size() - rows : 0u;
        scrollStart_ = std::min(scrollStart_, maximum);
        const std::size_t end = std::min(
            filtered.size(), scrollStart_ + rows);

        for (std::size_t i = scrollStart_; i < end; ++i)
        {
            const Message& message = *filtered[i];
            const std::string text = message.sender.empty()
                ? message.text
                : message.sender + ": " + message.text;

            Rml::ElementPtr row = document_->CreateElement("div");
            row->SetClass("chat-row", true);
            row->SetClass(MessageClassName(message.kind), true);

            Rml::ElementPtr shadow = document_->CreateElement("span");
            shadow->SetClass("chat-shadow", true);
            shadow->AppendChild(document_->CreateTextNode(text));

            Rml::ElementPtr line = document_->CreateElement("span");
            line->SetClass("chat-line", true);
            line->AppendChild(document_->CreateTextNode(text));

            row->AppendChild(std::move(shadow));
            row->AppendChild(std::move(line));
            messagesElement_->AppendChild(std::move(row));
        }

        (void)messageScroll_.SetState(
            scrollStart_, maximum, rows, rows, maximum > 0, true);
    }

    [[nodiscard]] std::size_t BlockedMaximumStart() const noexcept
    {
        return blockedNames_.size() > design_.blockedVisibleRows
            ? blockedNames_.size() - design_.blockedVisibleRows
            : 0u;
    }

    void RefreshBlockedRows(bool force)
    {
        if (!blockedList_ || (!blockedDirty_ && !force))
            return;

        blockedDirty_ = false;
        blockedRowButtons_.clear();
        blockedList_->SetInnerRML("");

        const std::size_t maximum = BlockedMaximumStart();
        blockedScrollStart_ = std::min(blockedScrollStart_, maximum);
        const std::size_t end = std::min(
            blockedNames_.size(),
            blockedScrollStart_ + design_.blockedVisibleRows);

        for (std::size_t absolute = blockedScrollStart_;
             absolute < end; ++absolute)
        {
            Rml::ElementPtr row = document_->CreateElement("div");
            row->SetClass("blocked-chat-user", true);
            row->SetClass("selected",
                          selectedBlocked_ && *selectedBlocked_ == absolute);
            row->AppendChild(
                document_->CreateTextNode(blockedNames_[absolute]));

            Rml::Element* raw = blockedList_->AppendChild(std::move(row));
            auto button = std::make_unique<RmlMuButton>();
            button->Bind(raw);
            blockedRowButtons_.push_back(std::move(button));
        }

        (void)blockedScroll_.SetState(
            blockedScrollStart_, maximum,
            design_.blockedVisibleRows, design_.blockedVisibleRows,
            maximum > 0, true);
    }

    RmlDocumentHost host_;
    Rml::ElementDocument* document_ = nullptr;

    Rml::Element* panel_ = nullptr;
    Rml::Element* view_ = nullptr;
    Rml::Element* background_ = nullptr;
    Rml::Element* title_ = nullptr;
    Rml::Element* messagesElement_ = nullptr;
    Rml::Element* menu_ = nullptr;
    Rml::Element* menuButtons_ = nullptr;
    Rml::Element* inputArea_ = nullptr;
    Rml::Element* scrollDownElement_ = nullptr;
    Rml::ElementFormControlInput* mainInput_ = nullptr;
    Rml::ElementFormControlInput* whisperInput_ = nullptr;

    Rml::Element* blockedPanel_ = nullptr;
    Rml::Element* blockedDrag_ = nullptr;
    Rml::Element* blockedList_ = nullptr;
    Rml::ElementFormControlInput* blockedInput_ = nullptr;

    RmlMuScrollBar messageScroll_;
    RmlMuScrollBar blockedScroll_;
    RmlMuMovablePanel blockedMover_;

    RmlMuButton configButton_;
    RmlMuButton normalButton_;
    RmlMuButton partyButton_;
    RmlMuButton guildButton_;
    RmlMuButton gensButton_;
    RmlMuButton whisperFilterButton_;
    RmlMuButton systemFilterButton_;
    RmlMuButton blockWhisperButton_;
    RmlMuButton viewModeButton_;
    RmlMuButton sizeButton_;
    RmlMuButton alphaButton_;
    RmlMuButton scrollDownButton_;
    RmlMuButton blockRegisterButton_;
    RmlMuButton blockDeleteButton_;
    RmlMuButton blockCloseButton_;
    std::vector<std::unique_ptr<RmlMuButton>> blockedRowButtons_;

    Design design_;
    std::vector<Message> messages_;
    std::vector<std::string> blockedNames_;

    OutgoingMode outgoingMode_ = OutgoingMode::Normal;
    bool whisperFilter_ = true;
    bool systemFilter_ = true;
    bool blockWhisper_ = false;
    bool frameVisible_ = true;
    bool blockedPanelVisible_ = false;
    bool visible_ = false;
    bool editing_ = false;

    float viewportHeight_ = 0.0f;
    std::size_t sizeIndex_ = 0;
    int alphaPercent_ = 60;
    std::size_t scrollStart_ = 0;
    std::size_t blockedScrollStart_ = 0;
    std::optional<std::size_t> selectedBlocked_;
    std::optional<std::string> blockRegisterRequest_;
    std::optional<std::string> blockDeleteRequest_;
    bool messagesDirty_ = true;
    bool blockedDirty_ = true;
};

RmlChatPanel::RmlChatPanel()
    : m_impl(std::make_unique<Impl>())
{
}

RmlChatPanel::~RmlChatPanel() = default;
RmlChatPanel::RmlChatPanel(RmlChatPanel&&) noexcept = default;
RmlChatPanel& RmlChatPanel::operator=(RmlChatPanel&&) noexcept = default;

bool RmlChatPanel::Load(bool show)
{
    return m_impl && m_impl->Load(show);
}

bool RmlChatPanel::Show()
{
    return m_impl && m_impl->Show();
}

bool RmlChatPanel::Hide()
{
    return m_impl && m_impl->Hide();
}

void RmlChatPanel::Release()
{
    if (m_impl)
        m_impl->Release();
}

bool RmlChatPanel::IsLoaded() const noexcept
{
    return m_impl && m_impl->IsLoaded();
}

void RmlChatPanel::SetVisible(bool visible)
{
    if (m_impl)
        m_impl->SetVisible(visible);
}

void RmlChatPanel::SetEditing(bool editing)
{
    if (m_impl)
        m_impl->SetEditing(editing);
}

void RmlChatPanel::SetViewportHeight(float viewportHeight)
{
    if (m_impl)
        m_impl->SetViewportHeight(viewportHeight);
}

void RmlChatPanel::ClearMessages()
{
    if (m_impl)
        m_impl->ClearMessages();
}

void RmlChatPanel::AddMessage(Message message)
{
    if (m_impl)
        m_impl->AddMessage(std::move(message));
}

void RmlChatPanel::SetMessages(std::vector<Message> messages)
{
    if (m_impl)
        m_impl->SetMessages(std::move(messages));
}

void RmlChatPanel::SetBlockedNames(std::vector<std::string> names)
{
    if (m_impl)
        m_impl->SetBlockedNames(std::move(names));
}

const std::vector<std::string>& RmlChatPanel::GetBlockedNames() const noexcept
{
    static const std::vector<std::string> empty;
    return m_impl ? m_impl->GetBlockedNames() : empty;
}

bool RmlChatPanel::Update()
{
    return m_impl && m_impl->Update();
}

RmlChatPanel::OutgoingMode RmlChatPanel::GetOutgoingMode() const noexcept
{
    return m_impl ? m_impl->GetOutgoingMode() : OutgoingMode::Normal;
}

bool RmlChatPanel::IsWhisperFilterEnabled() const noexcept
{
    return m_impl && m_impl->IsWhisperFilterEnabled();
}

bool RmlChatPanel::IsSystemFilterEnabled() const noexcept
{
    return m_impl && m_impl->IsSystemFilterEnabled();
}

bool RmlChatPanel::IsWhisperBlocked() const noexcept
{
    return m_impl && m_impl->IsWhisperBlocked();
}

int RmlChatPanel::GetAlphaPercent() const noexcept
{
    return m_impl ? m_impl->GetAlphaPercent() : 60;
}

std::size_t RmlChatPanel::GetSizeIndex() const noexcept
{
    return m_impl ? m_impl->GetSizeIndex() : 0u;
}

std::optional<std::string> RmlChatPanel::ConsumeBlockRegisterRequest()
{
    return m_impl
        ? m_impl->ConsumeBlockRegisterRequest()
        : std::nullopt;
}

std::optional<std::string> RmlChatPanel::ConsumeBlockDeleteRequest()
{
    return m_impl
        ? m_impl->ConsumeBlockDeleteRequest()
        : std::nullopt;
}

const char* RmlChatPanel::MessageClassName(MessageKind kind) noexcept
{
    // Recovered x64 switch at Main-x64-Debug.exe 0x1405361d0.
    switch (kind)
    {
    case MessageKind::Whisper: return "whisper";
    case MessageKind::System: return "system";
    case MessageKind::Error: return "error";
    case MessageKind::Party: return "party";
    case MessageKind::Guild: return "guild";
    case MessageKind::Union: return "union";
    case MessageKind::Gm: return "gm";
    case MessageKind::Gens: return "gens";
    case MessageKind::Normal:
    default: return "normal";
    }
}

} // namespace UI::Modern::PC::Chat

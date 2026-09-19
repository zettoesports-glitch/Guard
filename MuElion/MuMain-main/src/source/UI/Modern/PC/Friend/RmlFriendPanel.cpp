#include "stdafx.h"
#include "UI/Modern/PC/Friend/RmlFriendPanel.h"

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
#include <RmlUi/Core/Elements/ElementFormControl.h>

#include <algorithm>
#include <array>
#include <cstdio>
#include <sstream>
#include <utility>

namespace UI::Modern::PC::Friend
{

namespace
{
constexpr const char* kDocumentPath = "Data/UI/PC/Friend/friend.rml";

Rml::String PixelValue(float value)
{
    std::array<char, 64> buffer{};
    std::snprintf(
        buffer.data(), buffer.size(), "%.3fpx",
        static_cast<double>(value));
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

Rml::Element* ElementById(
    Rml::ElementDocument* document, const Rml::String& id)
{
    return document ? document->GetElementById(id) : nullptr;
}

Rml::ElementFormControl* FormById(
    Rml::ElementDocument* document, const Rml::String& id)
{
    return dynamic_cast<Rml::ElementFormControl*>(
        ElementById(document, id));
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

class RmlFriendPanel::Impl
{
public:
    struct Design
    {
        float mainWidth = 540.0f;
        float mainHeight = 381.0f;
        float chatWidth = 553.0f;
        float chatCollapsedWidth = 425.0f;
        float chatHeight = 282.0f;
        float writeWidth = 429.0f;
        float writeHeight = 282.0f;
        float readWidth = 425.0f;
        float readHeight = 278.0f;
        std::size_t mainVisibleRows = 5;
        std::size_t visibleRows = 8;
        float initialX = 50.0f;
        float initialY = 50.0f;
        std::size_t chatVisibleLines = 11;
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

        loadedVisible_ = show;
        return ApplyState(state_);
    }

    [[nodiscard]] bool Show()
    {
        if (!host_.IsLoaded() && !Load(false))
            return false;

        loadedVisible_ = true;
        const bool result = host_.Show();
        if (result)
            (void)ApplyState(state_);
        return result;
    }

    [[nodiscard]] bool Hide()
    {
        loadedVisible_ = false;
        HideAllPanels();
        return host_.Hide();
    }

    void Release()
    {
        mainScroll_.Unbind();

        mainMover_.Unbind();
        chatMover_.Unbind();
        writeMover_.Unbind();
        readMover_.Unbind();

        mainClose_.Unbind();
        refuseButton_.Unbind();
        headerCheckButton_.Unbind();
        for (auto& button : tabButtons_) button.Unbind();
        for (auto& button : mainActionButtons_) button.Unbind();
        for (auto& button : rowButtons_) button.Unbind();

        chatClose_.Unbind();
        chatMinimize_.Unbind();
        chatToggle_.Unbind();
        chatInvite_.Unbind();
        for (auto& button : inviteButtons_) button.Unbind();

        writeCloseTop_.Unbind();
        writeMinimize_.Unbind();
        writeSend_.Unbind();
        writeClose_.Unbind();

        readCloseTop_.Unbind();
        readMinimize_.Unbind();
        for (auto& button : readActionButtons_) button.Unbind();

        document_ = nullptr;
        main_ = chat_ = write_ = read_ = nullptr;
        mainDrag_ = chatDrag_ = writeDrag_ = readDrag_ = nullptr;
        mainTitle_ = chatTitle_ = writeTitle_ = readTitle_ = nullptr;
        refuseMark_ = headerCheckMark_ = nullptr;
        rowsContainer_ = nullptr;
        tabLabelElements_.fill(nullptr);
        headerElements_.fill(nullptr);
        actionLabelElements_.fill(nullptr);
        rowElements_.fill(nullptr);
        rowFieldElements_.fill({});
        rowCheckElements_.fill(nullptr);
        rowMailElements_.fill(nullptr);

        chatMessages_ = chatMembers_ = nullptr;
        chatInput_ = nullptr;
        inviteRowElements_.fill(nullptr);

        writeReceiver_ = writeSubject_ = writeBody_ = nullptr;
        writeReceiverLabel_ = writeSubjectLabel_ = nullptr;
        writeSendLabel_ = writeCloseLabel_ = nullptr;

        readSenderLabel_ = readSender_ = readBody_ = nullptr;
        readActionLabels_.fill(nullptr);

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
        if (state_.main.selectedTab >= MainTabCount)
            state_.main.selectedTab = 0;

        if (!IsLoaded())
            return false;

        const bool validViewport =
            state_.viewportWidth > 0 && state_.viewportHeight > 0;
        if (!validViewport || !loadedVisible_)
        {
            HideAllPanels();
            return true;
        }

        ApplyPanelVisibility();
        ApplyMain();
        ApplyChat();
        ApplyWrite();
        ApplyRead();

        document_->UpdateDocument();
        return true;
    }

    [[nodiscard]] bool Update()
    {
        if (!IsLoaded())
            return false;

        bool changed = false;

        changed |= UpdateMainControls();
        changed |= UpdateChatControls();
        changed |= UpdateWriteControls();
        changed |= UpdateReadControls();

        changed |= mainMover_.ConsumePositionChanged();
        changed |= chatMover_.ConsumePositionChanged();
        changed |= writeMover_.ConsumePositionChanged();
        changed |= readMover_.ConsumePositionChanged();

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

        (void)ParseScalar(values, "RmlFriendPanel-PanelMainWidth", design_.mainWidth);
        (void)ParseScalar(values, "RmlFriendPanel-PanelMainHeight", design_.mainHeight);
        (void)ParseScalar(values, "RmlFriendPanel-PanelChatWidth", design_.chatWidth);
        (void)ParseScalar(values, "RmlFriendPanel-PanelChatCollapsedWidth", design_.chatCollapsedWidth);
        (void)ParseScalar(values, "RmlFriendPanel-PanelChatHeight", design_.chatHeight);
        (void)ParseScalar(values, "RmlFriendPanel-PanelWriteWidth", design_.writeWidth);
        (void)ParseScalar(values, "RmlFriendPanel-PanelWriteHeight", design_.writeHeight);
        (void)ParseScalar(values, "RmlFriendPanel-PanelReadWidth", design_.readWidth);
        (void)ParseScalar(values, "RmlFriendPanel-PanelReadHeight", design_.readHeight);
        (void)ParseScalar(values, "RmlFriendPanel-PanelMainVisibleRows", design_.mainVisibleRows);
        (void)ParseScalar(values, "RmlFriendPanel-PanelVisibleRows", design_.visibleRows);
        (void)ParseScalar(values, "RmlFriendPanel-InitialX", design_.initialX);
        (void)ParseScalar(values, "RmlFriendPanel-InitialY", design_.initialY);
        (void)ParseScalar(values, "RmlFriendPanel-ChatVisibleLines", design_.chatVisibleLines);

        design_.mainVisibleRows =
            std::clamp<std::size_t>(design_.mainVisibleRows, 1, MainRowCount);
        design_.visibleRows =
            std::clamp<std::size_t>(design_.visibleRows, 1, MainRowCount);
    }

    [[nodiscard]] bool BindElements()
    {
        main_ = ElementById(document_, "friend-main");
        mainDrag_ = ElementById(document_, "friend-main-drag");
        mainTitle_ = ElementById(document_, "friend-main-title");
        refuseMark_ = ElementById(document_, "friend-refuse-mark");
        headerCheckMark_ = ElementById(document_, "friend-header-check-mark");
        rowsContainer_ = ElementById(document_, "friend-rows");

        chat_ = ElementById(document_, "friend-chat");
        chatDrag_ = ElementById(document_, "friend-chat-drag");
        chatTitle_ = ElementById(document_, "friend-chat-title");
        chatMessages_ = ElementById(document_, "friend-chat-messages");
        chatMembers_ = ElementById(document_, "friend-chat-members");
        chatInput_ = FormById(document_, "friend-chat-input");

        write_ = ElementById(document_, "friend-write");
        writeDrag_ = ElementById(document_, "friend-write-drag");
        writeTitle_ = ElementById(document_, "friend-write-title");
        writeReceiverLabel_ = ElementById(document_, "friend-write-receiver-label");
        writeSubjectLabel_ = ElementById(document_, "friend-write-subject-label");
        writeReceiver_ = FormById(document_, "friend-write-receiver");
        writeSubject_ = FormById(document_, "friend-write-subject");
        writeBody_ = FormById(document_, "friend-write-body");
        writeSendLabel_ = ElementById(document_, "friend-write-send-label");
        writeCloseLabel_ = ElementById(document_, "friend-write-close-label");

        read_ = ElementById(document_, "friend-read");
        readDrag_ = ElementById(document_, "friend-read-drag");
        readTitle_ = ElementById(document_, "friend-read-title");
        readSenderLabel_ = ElementById(document_, "friend-read-sender-label");
        readSender_ = ElementById(document_, "friend-read-sender");
        readBody_ = ElementById(document_, "friend-read-body");

        if (!main_ || !mainDrag_ || !mainTitle_ ||
            !refuseMark_ || !headerCheckMark_ || !rowsContainer_ ||
            !chat_ || !chatDrag_ || !chatTitle_ ||
            !chatMessages_ || !chatMembers_ || !chatInput_ ||
            !write_ || !writeDrag_ || !writeTitle_ ||
            !writeReceiverLabel_ || !writeSubjectLabel_ ||
            !writeReceiver_ || !writeSubject_ || !writeBody_ ||
            !writeSendLabel_ || !writeCloseLabel_ ||
            !read_ || !readDrag_ || !readTitle_ ||
            !readSenderLabel_ || !readSender_ || !readBody_)
            return false;

        mainClose_.Bind(ElementById(document_, "friend-main-close"));
        refuseButton_.Bind(ElementById(document_, "friend-refuse"));
        headerCheckButton_.Bind(ElementById(document_, "friend-header-check"));

        for (std::size_t i = 0; i < MainTabCount; ++i)
        {
            const std::string suffix = std::to_string(i);
            Rml::Element* element =
                ElementById(document_, "friend-tab-" + suffix);
            tabLabelElements_[i] =
                ElementById(document_, "friend-tab-label-" + suffix);
            if (!element || !tabLabelElements_[i])
                return false;
            tabButtons_[i].Bind(element);
        }

        for (std::size_t i = 0; i < headerElements_.size(); ++i)
        {
            headerElements_[i] =
                ElementById(document_, "friend-header-" + std::to_string(i));
            if (!headerElements_[i])
                return false;
        }

        for (std::size_t i = 0; i < MainActionCount; ++i)
        {
            const std::string suffix = std::to_string(i);
            Rml::Element* element =
                ElementById(document_, "friend-action-" + suffix);
            actionLabelElements_[i] =
                ElementById(document_, "friend-action-label-" + suffix);
            if (!element || !actionLabelElements_[i])
                return false;
            mainActionButtons_[i].Bind(element);
        }

        for (std::size_t i = 0; i < MainRowCount; ++i)
        {
            const std::string suffix = std::to_string(i);
            rowElements_[i] =
                ElementById(document_, "friend-row-" + suffix);
            if (!rowElements_[i])
                return false;

            rowButtons_[i].Bind(rowElements_[i]);

            for (std::size_t field = 0; field < 4; ++field)
            {
                Rml::ElementList children;
                rowElements_[i]->GetElementsByClassName(
                    children,
                    "data-grid-field-" + std::to_string(field));
                if (!children.empty())
                    rowFieldElements_[i][field] = children.front();
            }

            Rml::ElementList checks;
            rowElements_[i]->GetElementsByTagName(checks, "input");
            if (!checks.empty())
                rowCheckElements_[i] = checks.front();

            Rml::ElementList mails;
            rowElements_[i]->GetElementsByClassName(
                mails, "friend-mail-icon");
            if (!mails.empty())
                rowMailElements_[i] = mails.front();
        }

        if (!mainScroll_.Bind(ElementById(document_, "friend-scroll")))
            return false;

        chatClose_.Bind(ElementById(document_, "friend-chat-close"));
        chatMinimize_.Bind(ElementById(document_, "friend-chat-minimize"));
        chatToggle_.Bind(ElementById(document_, "friend-chat-toggle"));
        chatInvite_.Bind(ElementById(document_, "friend-chat-invite"));

        for (std::size_t i = 0; i < InviteRowCount; ++i)
        {
            inviteRowElements_[i] =
                ElementById(document_,
                    "friend-chat-invite-row-" + std::to_string(i));
            if (!inviteRowElements_[i])
                return false;
            inviteButtons_[i].Bind(inviteRowElements_[i]);
        }

        writeCloseTop_.Bind(ElementById(document_, "friend-write-close-top"));
        writeMinimize_.Bind(ElementById(document_, "friend-write-minimize"));
        writeSend_.Bind(ElementById(document_, "friend-write-send"));
        writeClose_.Bind(ElementById(document_, "friend-write-close"));

        readCloseTop_.Bind(ElementById(document_, "friend-read-close-top"));
        readMinimize_.Bind(ElementById(document_, "friend-read-minimize"));
        for (std::size_t i = 0; i < ReadActionCount; ++i)
        {
            Rml::Element* action =
                ElementById(document_, "friend-read-action-" + std::to_string(i));
            if (!action)
                return false;
            readActionButtons_[i].Bind(action);

            Rml::ElementList labels;
            action->GetElementsByClassName(
                labels, "mu-overflow-label");
            if (!labels.empty())
                readActionLabels_[i] = labels.front();
        }

        mainMover_.Bind(main_, mainDrag_);
        chatMover_.Bind(chat_, chatDrag_);
        writeMover_.Bind(write_, writeDrag_);
        readMover_.Bind(read_, readDrag_);

        mainMover_.SetPosition(design_.initialX, design_.initialY);
        chatMover_.SetPosition(design_.initialX + 20.0f, design_.initialY + 20.0f);
        writeMover_.SetPosition(design_.initialX + 40.0f, design_.initialY + 40.0f);
        readMover_.SetPosition(design_.initialX + 60.0f, design_.initialY + 60.0f);

        return true;
    }

    void ApplyPanelVisibility()
    {
        SetVisible(main_, state_.main.visible);
        SetVisible(chat_, state_.chat.visible);
        SetVisible(write_, state_.write.visible);
        SetVisible(read_, state_.read.visible);

        const float viewportWidth = static_cast<float>(state_.viewportWidth);
        const float viewportHeight = static_cast<float>(state_.viewportHeight);

        mainMover_.SetMetrics(
            viewportWidth, viewportHeight,
            design_.mainWidth, design_.mainHeight, 0.0f, 0.0f);

        chatMover_.SetMetrics(
            viewportWidth, viewportHeight,
            state_.chat.inviteOpen ? design_.chatWidth
                                   : design_.chatCollapsedWidth,
            design_.chatHeight, 0.0f, 0.0f);

        writeMover_.SetMetrics(
            viewportWidth, viewportHeight,
            design_.writeWidth, design_.writeHeight, 0.0f, 0.0f);

        readMover_.SetMetrics(
            viewportWidth, viewportHeight,
            design_.readWidth, design_.readHeight, 0.0f, 0.0f);
    }

    void ApplyMain()
    {
        if (!main_)
            return;

        SetText(mainTitle_, state_.main.title);

        for (std::size_t i = 0; i < MainTabCount; ++i)
        {
            SetText(tabLabelElements_[i], state_.main.tabLabels[i]);
            tabButtons_[i].SetEnabled(true);
            tabButtons_[i].SetVisible(true);
            (void)tabButtons_[i].Update();

            if (Rml::Element* element =
                    ElementById(document_, "friend-tab-" + std::to_string(i)))
                element->SetClass("selected", state_.main.selectedTab == i);
        }

        main_->SetClass("mail", state_.main.selectedTab == 2);
        main_->SetClass("window-list", state_.main.selectedTab == 1);

        for (std::size_t i = 0; i < headerElements_.size(); ++i)
            SetText(headerElements_[i], state_.main.headerLabels[i]);

        for (std::size_t i = 0; i < MainActionCount; ++i)
        {
            SetText(actionLabelElements_[i], state_.main.actionLabels[i]);
            mainActionButtons_[i].SetVisible(true);
            mainActionButtons_[i].SetEnabled(true);
            (void)mainActionButtons_[i].Update();
        }

        refuseMark_->SetAttribute("checked", state_.main.refuseChat);
        headerCheckMark_->SetAttribute("checked", state_.main.headerChecked);

        const std::size_t visibleRows =
            std::min(design_.visibleRows, MainRowCount);
        const std::size_t total = state_.main.rows.size();
        const std::size_t maximum =
            total > visibleRows ? total - visibleRows : 0;
        scrollStart_ = std::min(scrollStart_, maximum);

        for (std::size_t slot = 0; slot < MainRowCount; ++slot)
        {
            const std::size_t absolute = scrollStart_ + slot;
            const bool present =
                absolute < total && slot < visibleRows;

            if (!present)
            {
                rowButtons_[slot].SetVisible(false);
                (void)rowButtons_[slot].Update();
                continue;
            }

            const MainRow& row = state_.main.rows[absolute];
            rowButtons_[slot].SetVisible(row.visible);
            rowButtons_[slot].SetEnabled(row.visible && row.enabled);
            (void)rowButtons_[slot].Update();

            rowElements_[slot]->SetClass("mail", state_.main.selectedTab == 2);
            rowElements_[slot]->SetClass(
                "selected",
                state_.main.selectedRow &&
                *state_.main.selectedRow == absolute);

            for (std::size_t field = 0; field < 4; ++field)
                SetText(rowFieldElements_[slot][field], row.fields[field]);

            if (rowCheckElements_[slot])
                rowCheckElements_[slot]->SetAttribute("checked", row.checked);
            SetVisible(rowMailElements_[slot], row.hasMail);
        }

        (void)mainScroll_.SetState(
            scrollStart_, maximum, visibleRows, visibleRows,
            maximum > 0, true);
        (void)mainClose_.Update();
        (void)refuseButton_.Update();
        (void)headerCheckButton_.Update();
    }

    void ApplyChat()
    {
        if (!chat_)
            return;

        SetText(chatTitle_, state_.chat.title);
        chat_->SetClass("invite-open", state_.chat.inviteOpen);
        chat_->SetClass("minimized", state_.chat.minimized);

        SetLines(chatMessages_, state_.chat.messages, design_.chatVisibleLines);
        SetLines(chatMembers_, state_.chat.members, design_.visibleRows);

        for (std::size_t i = 0; i < InviteRowCount; ++i)
        {
            const bool present = i < state_.chat.inviteCandidates.size();
            inviteButtons_[i].SetVisible(present);
            inviteButtons_[i].SetEnabled(present);
            (void)inviteButtons_[i].Update();

            if (present)
                SetText(inviteRowElements_[i], state_.chat.inviteCandidates[i]);
            else
                SetText(inviteRowElements_[i], "");

            inviteRowElements_[i]->SetClass(
                "selected",
                state_.chat.selectedInvite &&
                *state_.chat.selectedInvite == i);
        }

        if (Rml::Element* toggleLabel =
                ElementById(document_, "friend-chat-toggle-label"))
            SetText(toggleLabel, state_.chat.toggleLabel);
        if (Rml::Element* inviteLabel =
                ElementById(document_, "friend-chat-invite-label"))
            SetText(inviteLabel, state_.chat.inviteLabel);

        (void)chatClose_.Update();
        (void)chatMinimize_.Update();
        (void)chatToggle_.Update();
        (void)chatInvite_.Update();
    }

    void ApplyWrite()
    {
        if (!write_)
            return;

        write_->SetClass("minimized", state_.write.minimized);
        SetText(writeTitle_, state_.write.title);
        SetText(writeReceiverLabel_, state_.write.receiverLabel);
        SetText(writeSubjectLabel_, state_.write.subjectLabel);
        SetText(writeSendLabel_, state_.write.sendLabel);
        SetText(writeCloseLabel_, state_.write.closeLabel);

        if (writeReceiver_) writeReceiver_->SetValue(state_.write.receiver);
        if (writeSubject_) writeSubject_->SetValue(state_.write.subject);
        if (writeBody_) writeBody_->SetValue(state_.write.body);

        (void)writeCloseTop_.Update();
        (void)writeMinimize_.Update();
        (void)writeSend_.Update();
        (void)writeClose_.Update();
    }

    void ApplyRead()
    {
        if (!read_)
            return;

        read_->SetClass("minimized", state_.read.minimized);
        SetText(readTitle_, state_.read.title);
        SetText(readSenderLabel_, state_.read.senderLabel);
        SetText(readSender_, state_.read.sender);
        SetText(readBody_, state_.read.body);

        for (std::size_t i = 0; i < ReadActionCount; ++i)
        {
            SetText(readActionLabels_[i], state_.read.actionLabels[i]);
            readActionButtons_[i].SetVisible(true);
            readActionButtons_[i].SetEnabled(state_.read.actionEnabled[i]);
            (void)readActionButtons_[i].Update();
        }

        (void)readCloseTop_.Update();
        (void)readMinimize_.Update();
    }

    void SetLines(
        Rml::Element* container,
        const std::vector<std::string>& lines,
        std::size_t maximumLines)
    {
        if (!container)
            return;

        container->SetInnerRML("");
        if (!document_)
            return;

        const std::size_t begin =
            lines.size() > maximumLines ? lines.size() - maximumLines : 0;

        for (std::size_t i = begin; i < lines.size(); ++i)
        {
            Rml::ElementPtr line = document_->CreateElement("div");
            line->SetClass("friend-text-line", true);
            line->AppendChild(document_->CreateTextNode(lines[i]));
            container->AppendChild(std::move(line));
        }
    }

    [[nodiscard]] bool UpdateMainControls()
    {
        bool changed = false;

        changed |= mainClose_.Update();
        if (mainClose_.ConsumeClicked())
        {
            pendingAction_ = Action{ActionType::CloseMain};
            changed = true;
        }

        changed |= refuseButton_.Update();
        if (refuseButton_.ConsumeClicked())
        {
            pendingAction_ = Action{ActionType::ToggleRefuse};
            changed = true;
        }

        changed |= headerCheckButton_.Update();
        if (headerCheckButton_.ConsumeClicked())
        {
            pendingAction_ = Action{ActionType::ToggleHeaderCheck};
            changed = true;
        }

        for (std::size_t i = 0; i < MainTabCount; ++i)
        {
            changed |= tabButtons_[i].Update();
            if (tabButtons_[i].ConsumeClicked())
            {
                pendingAction_ =
                    Action{ActionType::ChangeTab, i};
                changed = true;
            }
        }

        for (std::size_t i = 0; i < MainActionCount; ++i)
        {
            changed |= mainActionButtons_[i].Update();
            if (mainActionButtons_[i].ConsumeClicked())
            {
                pendingAction_ =
                    Action{ActionType::MainAction, i};
                changed = true;
            }
        }

        if (const auto request = mainScroll_.ConsumeRequestedPosition())
        {
            scrollStart_ = *request;
            ApplyMain();
            changed = true;
        }

        for (std::size_t slot = 0; slot < MainRowCount; ++slot)
        {
            changed |= rowButtons_[slot].Update();
            if (rowButtons_[slot].ConsumeClicked())
            {
                const std::size_t absolute = scrollStart_ + slot;
                if (absolute < state_.main.rows.size())
                {
                    pendingAction_ =
                        Action{ActionType::SelectMainRow, absolute};
                    changed = true;
                }
            }
        }

        return changed;
    }

    [[nodiscard]] bool UpdateChatControls()
    {
        bool changed = false;

        changed |= chatClose_.Update();
        changed |= chatMinimize_.Update();
        changed |= chatToggle_.Update();
        changed |= chatInvite_.Update();

        if (chatClose_.ConsumeClicked())
        {
            pendingAction_ = Action{ActionType::CloseChat};
            changed = true;
        }
        if (chatMinimize_.ConsumeClicked())
        {
            pendingAction_ = Action{ActionType::MinimizeChat};
            changed = true;
        }
        if (chatToggle_.ConsumeClicked())
        {
            pendingAction_ = Action{ActionType::ToggleInvite};
            changed = true;
        }
        if (chatInvite_.ConsumeClicked())
        {
            pendingAction_ = Action{ActionType::InviteSelected};
            changed = true;
        }

        for (std::size_t i = 0; i < InviteRowCount; ++i)
        {
            changed |= inviteButtons_[i].Update();
            if (inviteButtons_[i].ConsumeClicked())
            {
                pendingAction_ =
                    Action{ActionType::SelectInviteRow, i};
                changed = true;
            }
        }

        return changed;
    }

    [[nodiscard]] bool UpdateWriteControls()
    {
        bool changed = false;
        changed |= writeCloseTop_.Update();
        changed |= writeMinimize_.Update();
        changed |= writeSend_.Update();
        changed |= writeClose_.Update();

        if (writeCloseTop_.ConsumeClicked() ||
            writeClose_.ConsumeClicked())
        {
            pendingAction_ = Action{ActionType::CloseWrite};
            changed = true;
        }

        if (writeMinimize_.ConsumeClicked())
        {
            pendingAction_ = Action{ActionType::MinimizeWrite};
            changed = true;
        }

        if (writeSend_.ConsumeClicked())
        {
            Action action{ActionType::SendLetter};
            action.text = writeBody_ ? writeBody_->GetValue() : "";
            if (writeReceiver_)
                action.auxiliary = writeReceiver_->GetValue();
            if (writeSubject_)
            {
                if (!action.auxiliary.empty())
                    action.auxiliary += "\n";
                action.auxiliary += writeSubject_->GetValue();
            }
            pendingAction_ = std::move(action);
            changed = true;
        }

        return changed;
    }

    [[nodiscard]] bool UpdateReadControls()
    {
        bool changed = false;
        changed |= readCloseTop_.Update();
        changed |= readMinimize_.Update();

        if (readCloseTop_.ConsumeClicked())
        {
            pendingAction_ = Action{ActionType::CloseRead};
            changed = true;
        }

        if (readMinimize_.ConsumeClicked())
        {
            pendingAction_ = Action{ActionType::MinimizeRead};
            changed = true;
        }

        for (std::size_t i = 0; i < ReadActionCount; ++i)
        {
            changed |= readActionButtons_[i].Update();
            if (readActionButtons_[i].ConsumeClicked())
            {
                pendingAction_ =
                    Action{ActionType::ReadAction, i};
                changed = true;
            }
        }

        return changed;
    }

    void HideAllPanels()
    {
        SetVisible(main_, false);
        SetVisible(chat_, false);
        SetVisible(write_, false);
        SetVisible(read_, false);
    }

    RmlDocumentHost host_;
    Rml::ElementDocument* document_ = nullptr;
    bool loadedVisible_ = false;

    Rml::Element* main_ = nullptr;
    Rml::Element* mainDrag_ = nullptr;
    Rml::Element* mainTitle_ = nullptr;
    Rml::Element* refuseMark_ = nullptr;
    Rml::Element* headerCheckMark_ = nullptr;
    Rml::Element* rowsContainer_ = nullptr;

    std::array<Rml::Element*, MainTabCount> tabLabelElements_{};
    std::array<Rml::Element*, 4> headerElements_{};
    std::array<Rml::Element*, MainActionCount> actionLabelElements_{};
    std::array<Rml::Element*, MainRowCount> rowElements_{};
    std::array<std::array<Rml::Element*, 4>, MainRowCount> rowFieldElements_{};
    std::array<Rml::Element*, MainRowCount> rowCheckElements_{};
    std::array<Rml::Element*, MainRowCount> rowMailElements_{};

    Rml::Element* chat_ = nullptr;
    Rml::Element* chatDrag_ = nullptr;
    Rml::Element* chatTitle_ = nullptr;
    Rml::Element* chatMessages_ = nullptr;
    Rml::Element* chatMembers_ = nullptr;
    Rml::ElementFormControl* chatInput_ = nullptr;
    std::array<Rml::Element*, InviteRowCount> inviteRowElements_{};

    Rml::Element* write_ = nullptr;
    Rml::Element* writeDrag_ = nullptr;
    Rml::Element* writeTitle_ = nullptr;
    Rml::Element* writeReceiverLabel_ = nullptr;
    Rml::Element* writeSubjectLabel_ = nullptr;
    Rml::ElementFormControl* writeReceiver_ = nullptr;
    Rml::ElementFormControl* writeSubject_ = nullptr;
    Rml::ElementFormControl* writeBody_ = nullptr;
    Rml::Element* writeSendLabel_ = nullptr;
    Rml::Element* writeCloseLabel_ = nullptr;

    Rml::Element* read_ = nullptr;
    Rml::Element* readDrag_ = nullptr;
    Rml::Element* readTitle_ = nullptr;
    Rml::Element* readSenderLabel_ = nullptr;
    Rml::Element* readSender_ = nullptr;
    Rml::Element* readBody_ = nullptr;
    std::array<Rml::Element*, ReadActionCount> readActionLabels_{};

    std::array<RmlMuButton, MainTabCount> tabButtons_{};
    std::array<RmlMuButton, MainActionCount> mainActionButtons_{};
    std::array<RmlMuButton, MainRowCount> rowButtons_{};
    RmlMuButton mainClose_;
    RmlMuButton refuseButton_;
    RmlMuButton headerCheckButton_;
    RmlMuScrollBar mainScroll_;
    RmlMuMovablePanel mainMover_;

    RmlMuButton chatClose_;
    RmlMuButton chatMinimize_;
    RmlMuButton chatToggle_;
    RmlMuButton chatInvite_;
    std::array<RmlMuButton, InviteRowCount> inviteButtons_{};
    RmlMuMovablePanel chatMover_;

    RmlMuButton writeCloseTop_;
    RmlMuButton writeMinimize_;
    RmlMuButton writeSend_;
    RmlMuButton writeClose_;
    RmlMuMovablePanel writeMover_;

    RmlMuButton readCloseTop_;
    RmlMuButton readMinimize_;
    std::array<RmlMuButton, ReadActionCount> readActionButtons_{};
    RmlMuMovablePanel readMover_;

    Design design_;
    State state_;
    std::size_t scrollStart_ = 0;
    std::optional<Action> pendingAction_;
};

RmlFriendPanel::RmlFriendPanel()
    : m_impl(std::make_unique<Impl>())
{
}

RmlFriendPanel::~RmlFriendPanel() = default;
RmlFriendPanel::RmlFriendPanel(RmlFriendPanel&&) noexcept = default;
RmlFriendPanel& RmlFriendPanel::operator=(RmlFriendPanel&&) noexcept = default;

bool RmlFriendPanel::Load(bool show)
{
    return m_impl && m_impl->Load(show);
}
bool RmlFriendPanel::Show()
{
    return m_impl && m_impl->Show();
}
bool RmlFriendPanel::Hide()
{
    return m_impl && m_impl->Hide();
}
void RmlFriendPanel::Release()
{
    if (m_impl) m_impl->Release();
}
bool RmlFriendPanel::IsLoaded() const noexcept
{
    return m_impl && m_impl->IsLoaded();
}
bool RmlFriendPanel::ApplyState(const State& state)
{
    return m_impl && m_impl->ApplyState(state);
}
bool RmlFriendPanel::Update()
{
    return m_impl && m_impl->Update();
}
std::optional<RmlFriendPanel::Action> RmlFriendPanel::ConsumeAction()
{
    return m_impl ? m_impl->ConsumeAction() : std::nullopt;
}

} // namespace UI::Modern::PC::Friend

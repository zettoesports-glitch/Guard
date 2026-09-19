#pragma once

#include <array>
#include <cstddef>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace UI::Modern::PC::Friend
{

class RmlFriendPanel
{
public:
    static constexpr std::size_t MainRowCount = 8;
    static constexpr std::size_t MainTabCount = 3;
    static constexpr std::size_t MainActionCount = 4;
    static constexpr std::size_t InviteRowCount = 8;
    static constexpr std::size_t ReadActionCount = 5;

    enum class ActionType
    {
        CloseMain,
        ChangeTab,
        ToggleRefuse,
        ToggleHeaderCheck,
        SelectMainRow,
        MainAction,
        CloseChat,
        MinimizeChat,
        ToggleInvite,
        SelectInviteRow,
        InviteSelected,
        SubmitChat,
        CloseWrite,
        MinimizeWrite,
        SendLetter,
        CloseRead,
        MinimizeRead,
        ReadAction,
    };

    struct Action
    {
        ActionType type = ActionType::CloseMain;
        std::size_t index = 0;
        std::string text;
        std::string auxiliary;
    };

    struct MainRow
    {
        std::array<std::string, 4> fields{};
        bool visible = true;
        bool enabled = true;
        bool checked = false;
        bool hasMail = false;
    };

    struct MainState
    {
        bool visible = false;
        std::string title = "Friend";
        std::array<std::string, MainTabCount> tabLabels{
            "Friends", "Chat", "Mail"};
        std::array<std::string, 4> headerLabels{};
        std::array<std::string, MainActionCount> actionLabels{};
        std::vector<MainRow> rows;
        std::size_t selectedTab = 0;
        std::optional<std::size_t> selectedRow;
        bool refuseChat = false;
        bool headerChecked = false;
    };

    struct ChatState
    {
        bool visible = false;
        bool minimized = false;
        bool inviteOpen = false;
        std::string title;
        std::vector<std::string> messages;
        std::vector<std::string> members;
        std::vector<std::string> inviteCandidates;
        std::optional<std::size_t> selectedInvite;
        std::string toggleLabel = "Members";
        std::string inviteLabel = "Invite";
    };

    struct WriteState
    {
        bool visible = false;
        bool minimized = false;
        std::string title = "Write Mail";
        std::string receiverLabel = "To";
        std::string subjectLabel = "Subject";
        std::string receiver;
        std::string subject;
        std::string body;
        std::string sendLabel = "Send";
        std::string closeLabel = "Close";
    };

    struct ReadState
    {
        bool visible = false;
        bool minimized = false;
        std::string title = "Mail";
        std::string senderLabel = "From";
        std::string sender;
        std::string body;
        std::array<std::string, ReadActionCount> actionLabels{};
        std::array<bool, ReadActionCount> actionEnabled{
            true, true, true, true, true};
    };

    struct State
    {
        int viewportWidth = 0;
        int viewportHeight = 0;

        MainState main;
        ChatState chat;
        WriteState write;
        ReadState read;
    };

    RmlFriendPanel();
    ~RmlFriendPanel();

    RmlFriendPanel(const RmlFriendPanel&) = delete;
    RmlFriendPanel& operator=(const RmlFriendPanel&) = delete;
    RmlFriendPanel(RmlFriendPanel&&) noexcept;
    RmlFriendPanel& operator=(RmlFriendPanel&&) noexcept;

    [[nodiscard]] bool Load(bool show = false);
    [[nodiscard]] bool Show();
    [[nodiscard]] bool Hide();
    void Release();

    [[nodiscard]] bool IsLoaded() const noexcept;
    [[nodiscard]] bool ApplyState(const State& state);
    [[nodiscard]] bool Update();

    [[nodiscard]] std::optional<Action> ConsumeAction();

private:
    class Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace UI::Modern::PC::Friend

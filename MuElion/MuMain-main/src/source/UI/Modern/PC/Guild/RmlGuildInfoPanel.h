#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace UI::Modern::PC::Guild
{

class RmlGuildInfoPanel
{
public:
    static constexpr std::size_t MarkCellCount = 64;

    enum class Tab : std::uint8_t { Info, Members, Union, Rival };
    enum class ActionType : std::uint8_t
    {
        Close,
        SelectTab,
        SelectMember,
        SelectUnion,
        SelectRival,
        Position,
        ClearPosition,
        FireMember,
        DisbandOrLeave,
        UnionDisbandGuild,
        UnionDisbandUnion,
        RivalAdd,
        RivalDelete,
    };

    struct Member
    {
        std::string name;
        std::string position;
        std::string server;
    };
    struct UnionGuild
    {
        std::string name;
        int memberCount = 0;
        std::array<std::uint8_t, MarkCellCount> mark{};
    };
    struct Action
    {
        ActionType type = ActionType::Close;
        std::size_t index = 0;
        Tab tab = Tab::Info;
    };
    struct State
    {
        bool visible = false;
        int viewportWidth = 0;
        int viewportHeight = 0;
        float x = 20.0f;
        float y = 20.0f;
        std::string title = "Guild";
        bool hasGuild = false;
        bool isGuildMaster = false;
        Tab tab = Tab::Members;
        std::string guildName;
        int guildScore = 0;
        std::size_t memberCount = 0;
        std::size_t memberCapacity = 0;
        std::string rivalGuild;
        std::string notice;
        std::array<std::uint8_t, MarkCellCount> guildMark{};
        std::vector<Member> members;
        std::vector<UnionGuild> unions;
        std::vector<std::string> rivals;
    };

    RmlGuildInfoPanel();
    ~RmlGuildInfoPanel();
    RmlGuildInfoPanel(const RmlGuildInfoPanel&) = delete;
    RmlGuildInfoPanel& operator=(const RmlGuildInfoPanel&) = delete;
    RmlGuildInfoPanel(RmlGuildInfoPanel&&) noexcept;
    RmlGuildInfoPanel& operator=(RmlGuildInfoPanel&&) noexcept;

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

} // namespace UI::Modern::PC::Guild

#pragma once

#include <cstdint>
#include <memory>
#include <optional>
#include <string>

namespace UI::Modern::PC::NPC
{

class RmlGatekeeperPanel
{
public:
    enum class Mode : std::uint8_t
    {
        Guest,
        GuildMember,
        GuildMaster,
    };

    enum class ActionType : std::uint8_t
    {
        Close,
        Enter,
        TogglePublic,
        SetFee,
        FeeChanged,
    };

    struct Action
    {
        ActionType type = ActionType::Close;
        bool flag = false;
        int value = 0;
    };

    struct State
    {
        bool visible = false;
        int viewportWidth = 0;
        int viewportHeight = 0;
        float x = 20.0f;
        float y = 20.0f;
        Mode mode = Mode::Guest;
        bool publicEntry = false;
        int entranceFee = 0;
        int viewEntranceFee = 0;
        int addEntranceFee = 1;
        int maxEntranceFee = 0;
        int playerGold = 0;
        std::string title = "Gatekeeper";
    };

    RmlGatekeeperPanel();
    ~RmlGatekeeperPanel();
    RmlGatekeeperPanel(const RmlGatekeeperPanel&) = delete;
    RmlGatekeeperPanel& operator=(const RmlGatekeeperPanel&) = delete;
    RmlGatekeeperPanel(RmlGatekeeperPanel&&) noexcept;
    RmlGatekeeperPanel& operator=(RmlGatekeeperPanel&&) noexcept;

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

} // namespace UI::Modern::PC::NPC

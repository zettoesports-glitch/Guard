#pragma once
#include <array>
#include <memory>
#include <optional>
#include <string>

namespace UI::Modern::PC::Combat
{
class RmlDuelWatchPanel
{
public:
    struct Channel
    {
        bool enabled=false;
        bool joinable=false;
        std::string user1;
        std::string user2;
    };
    struct State
    {
        bool visible=false;
        int viewportWidth=0;
        int viewportHeight=0;
        float x=0.0f;
        float y=0.0f;
        std::string title="Duel Watch";
        std::string explain="Select an arena to watch";
        std::array<Channel,4> channels{};
    };

    RmlDuelWatchPanel();
    ~RmlDuelWatchPanel();
    RmlDuelWatchPanel(const RmlDuelWatchPanel&)=delete;
    RmlDuelWatchPanel& operator=(const RmlDuelWatchPanel&)=delete;
    RmlDuelWatchPanel(RmlDuelWatchPanel&&) noexcept;
    RmlDuelWatchPanel& operator=(RmlDuelWatchPanel&&) noexcept;

    [[nodiscard]] bool Load(bool show=false);
    [[nodiscard]] bool Show();
    [[nodiscard]] bool Hide();
    void Release();
    [[nodiscard]] bool IsLoaded() const noexcept;
    [[nodiscard]] bool ApplyState(const State& state);
    [[nodiscard]] bool Update();
    [[nodiscard]] std::optional<int> ConsumeJoinChannel();
    [[nodiscard]] bool ConsumeCloseRequest() noexcept;

private:
    class Impl;
    std::unique_ptr<Impl> m_impl;
};
}

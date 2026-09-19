#pragma once
#include <memory>
#include <string>

namespace UI::Modern::PC::Combat
{
class RmlDuelResultPanel
{
public:
    struct State
    {
        bool visible=false;
        std::string title="Duel Result";
        std::string info;
        std::string closeLabel="OK";
    };

    RmlDuelResultPanel();
    ~RmlDuelResultPanel();
    RmlDuelResultPanel(const RmlDuelResultPanel&)=delete;
    RmlDuelResultPanel& operator=(const RmlDuelResultPanel&)=delete;
    RmlDuelResultPanel(RmlDuelResultPanel&&) noexcept;
    RmlDuelResultPanel& operator=(RmlDuelResultPanel&&) noexcept;

    [[nodiscard]] bool Load(bool show=false);
    [[nodiscard]] bool Show();
    [[nodiscard]] bool Hide();
    void Release();
    [[nodiscard]] bool IsLoaded() const noexcept;
    [[nodiscard]] bool ApplyState(const State& state);
    [[nodiscard]] bool Update();
    [[nodiscard]] bool ConsumeCloseRequest() noexcept;

private:
    class Impl;
    std::unique_ptr<Impl> m_impl;
};
}

#pragma once

#include <array>
#include <memory>
#include <optional>
#include <string>

namespace UI::Modern::PC::SystemMenu
{
class RmlWindowMenuPanel
{
public:
    enum class Action { System=0, Help, Guild, MoveMap, MiniMap, Gens };
    struct State
    {
        bool visible=false;
        int viewportWidth=0;
        int viewportHeight=0;
        float x=0.0f;
        float y=0.0f;
        std::array<bool,6> enabled{true,true,true,true,true,true};
        std::array<std::string,6> labels{"System","Help","Guild","Move","Map","Gens"};
    };

    RmlWindowMenuPanel();
    ~RmlWindowMenuPanel();
    RmlWindowMenuPanel(const RmlWindowMenuPanel&)=delete;
    RmlWindowMenuPanel& operator=(const RmlWindowMenuPanel&)=delete;
    RmlWindowMenuPanel(RmlWindowMenuPanel&&) noexcept;
    RmlWindowMenuPanel& operator=(RmlWindowMenuPanel&&) noexcept;

    [[nodiscard]] bool Load(bool show=false);
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
} // namespace UI::Modern::PC::SystemMenu

#pragma once

#include <memory>
#include <optional>
#include <string>

namespace UI::Modern::PC::HUD
{

class RmlTopMenuLayer
{
public:
    enum class Action
    {
        OpenOptions,
        ToggleHelper,
    };

    struct State
    {
        bool visible = false;
        int viewportWidth = 0;
        int viewportHeight = 0;

        std::string mapName;
        int mapX = 0;
        int mapY = 0;

        bool optionEnabled = true;
        bool actionEnabled = true;
        bool helperRunning = false;
    };

    RmlTopMenuLayer();
    ~RmlTopMenuLayer();

    RmlTopMenuLayer(const RmlTopMenuLayer&) = delete;
    RmlTopMenuLayer& operator=(const RmlTopMenuLayer&) = delete;
    RmlTopMenuLayer(RmlTopMenuLayer&&) noexcept;
    RmlTopMenuLayer& operator=(RmlTopMenuLayer&&) noexcept;

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

} // namespace UI::Modern::PC::HUD

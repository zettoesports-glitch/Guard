#pragma once

#include <memory>
#include <string>
#include <vector>

namespace UI::Modern::PC::World
{

class RmlMonsterInfoLayer
{
public:
    struct Monster
    {
        bool visible = true;
        std::string name;
        float hpFraction = 1.0f;
        float screenX = 0.0f;
        float screenY = 0.0f;
        bool target = false;
    };

    struct State
    {
        bool visible = false;
        int viewportWidth = 0;
        int viewportHeight = 0;
        std::vector<Monster> monsters;
    };

    RmlMonsterInfoLayer();
    ~RmlMonsterInfoLayer();

    RmlMonsterInfoLayer(const RmlMonsterInfoLayer&) = delete;
    RmlMonsterInfoLayer& operator=(const RmlMonsterInfoLayer&) = delete;
    RmlMonsterInfoLayer(RmlMonsterInfoLayer&&) noexcept;
    RmlMonsterInfoLayer& operator=(RmlMonsterInfoLayer&&) noexcept;

    [[nodiscard]] bool Load(bool show = false);
    [[nodiscard]] bool Show();
    [[nodiscard]] bool Hide();
    void Release();

    [[nodiscard]] bool IsLoaded() const noexcept;
    [[nodiscard]] bool ApplyState(State state);

private:
    class Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace UI::Modern::PC::World

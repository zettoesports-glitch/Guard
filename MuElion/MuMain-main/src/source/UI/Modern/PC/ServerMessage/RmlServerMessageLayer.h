#pragma once

#include <memory>
#include <string>

namespace UI::Modern::PC::ServerMessage
{

class RmlServerMessageLayer
{
public:
    struct State
    {
        bool visible = false;
        int viewportWidth = 0;
        int viewportHeight = 0;
        std::string text;
    };

    RmlServerMessageLayer();
    ~RmlServerMessageLayer();

    RmlServerMessageLayer(const RmlServerMessageLayer&) = delete;
    RmlServerMessageLayer& operator=(const RmlServerMessageLayer&) = delete;
    RmlServerMessageLayer(RmlServerMessageLayer&&) noexcept;
    RmlServerMessageLayer& operator=(RmlServerMessageLayer&&) noexcept;

    [[nodiscard]] bool Load(bool show = false);
    [[nodiscard]] bool Show();
    [[nodiscard]] bool Hide();
    void Release();

    [[nodiscard]] bool IsLoaded() const noexcept;
    [[nodiscard]] bool ApplyState(const State& state);

private:
    class Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace UI::Modern::PC::ServerMessage

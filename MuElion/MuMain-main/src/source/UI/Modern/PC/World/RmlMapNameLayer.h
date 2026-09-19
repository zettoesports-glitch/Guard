#pragma once

#include <memory>
#include <string>

namespace UI::Modern::PC::World
{

class RmlMapNameLayer
{
public:
    RmlMapNameLayer();
    ~RmlMapNameLayer();

    RmlMapNameLayer(const RmlMapNameLayer&) = delete;
    RmlMapNameLayer& operator=(const RmlMapNameLayer&) = delete;
    RmlMapNameLayer(RmlMapNameLayer&&) noexcept;
    RmlMapNameLayer& operator=(RmlMapNameLayer&&) noexcept;

    [[nodiscard]] bool Load(bool show = false);
    [[nodiscard]] bool Show();
    [[nodiscard]] bool Hide();
    void Release();

    [[nodiscard]] bool IsLoaded() const noexcept;
    void SetViewport(int width, int height) noexcept;

    // Starts the recovered 0.5s fade-in, 5s hold, 0.5s fade-out sequence.
    [[nodiscard]] bool ShowMapName(
        std::string mapName,
        std::string occupyGuild = {},
        bool strife = false);
    [[nodiscard]] bool Update();

private:
    class Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace UI::Modern::PC::World

#pragma once

#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace UI::Modern::PC::HUD
{

class RmlBuffListLayer
{
public:
    struct Entry
    {
        int buffState = 0;
        bool debuff = false;
        int referenceCount = 0;
        std::string remainingTime;
    };

    struct State
    {
        bool visible = false;
        int viewportWidth = 0;
        int viewportHeight = 0;
        std::vector<Entry> entries;
    };

    RmlBuffListLayer();
    ~RmlBuffListLayer();

    RmlBuffListLayer(const RmlBuffListLayer&) = delete;
    RmlBuffListLayer& operator=(const RmlBuffListLayer&) = delete;
    RmlBuffListLayer(RmlBuffListLayer&&) noexcept;
    RmlBuffListLayer& operator=(RmlBuffListLayer&&) noexcept;

    [[nodiscard]] bool Load(bool show = false);
    [[nodiscard]] bool Show();
    [[nodiscard]] bool Hide();
    void Release();

    [[nodiscard]] bool IsLoaded() const noexcept;
    void SetViewport(int width, int height) noexcept;
    [[nodiscard]] bool ApplyState(State state);
    [[nodiscard]] bool Update();

    // Clicking an icon returns the legacy buff-state value. Game-side decides
    // whether that buff is cancellable.
    [[nodiscard]] std::optional<int> ConsumeClickedBuff();

private:
    class Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace UI::Modern::PC::HUD

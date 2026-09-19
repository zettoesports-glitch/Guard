#pragma once

#include <array>
#include <memory>
#include <optional>
#include <string>

namespace UI::Modern::PC::Character
{

class RmlPetFrameLayer
{
public:
    enum class Action
    {
        ToggleMinimized,
    };

    struct Member
    {
        bool visible = false;
        std::string name;
        float healthRatio = 0.0f;
        float yellowHealthRatio = 0.0f;
        bool showYellowHealth = false;
    };

    struct State
    {
        bool visible = false;
        bool minimized = false;

        int viewportWidth = 0;
        int viewportHeight = 0;

        bool applyPosition = false;
        float positionX = 0.0f;
        float positionY = 0.0f;

        std::array<Member, 5> members{};
    };

    RmlPetFrameLayer();
    ~RmlPetFrameLayer();

    RmlPetFrameLayer(const RmlPetFrameLayer&) = delete;
    RmlPetFrameLayer& operator=(const RmlPetFrameLayer&) = delete;
    RmlPetFrameLayer(RmlPetFrameLayer&&) noexcept;
    RmlPetFrameLayer& operator=(RmlPetFrameLayer&&) noexcept;

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

} // namespace UI::Modern::PC::Character

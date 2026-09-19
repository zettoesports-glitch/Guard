#pragma once

#include <array>
#include <cstddef>
#include <memory>
#include <optional>
#include <string>

namespace UI::Modern::PC::Party
{

class RmlPartyFrameLayer
{
public:
    static constexpr std::size_t MaxMembers = 5;

    enum class ActionType
    {
        ToggleMinimize,
        SelectMember,
        LeaveMember,
    };

    struct Action
    {
        ActionType type = ActionType::SelectMember;
        std::size_t memberIndex = 0;
    };

    struct Member
    {
        bool visible = false;
        bool leader = false;
        bool canLeave = false;
        bool outOfSight = false;
        bool selected = false;

        std::string channel;
        std::string name;

        float hpRatio = 0.0f;
        float mpRatio = 0.0f;
    };

    struct State
    {
        bool visible = false;
        bool minimized = false;

        int viewportWidth = 0;
        int viewportHeight = 0;

        bool applyPosition = false;
        float positionX = 0.0f;
        float positionY = 38.0f;

        std::array<Member, MaxMembers> members{};
    };

    RmlPartyFrameLayer();
    ~RmlPartyFrameLayer();

    RmlPartyFrameLayer(const RmlPartyFrameLayer&) = delete;
    RmlPartyFrameLayer& operator=(const RmlPartyFrameLayer&) = delete;
    RmlPartyFrameLayer(RmlPartyFrameLayer&&) noexcept;
    RmlPartyFrameLayer& operator=(RmlPartyFrameLayer&&) noexcept;

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

} // namespace UI::Modern::PC::Party

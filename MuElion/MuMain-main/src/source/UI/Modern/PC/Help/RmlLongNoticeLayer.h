#pragma once

#include <cstdint>
#include <memory>
#include <string>

namespace UI::Modern::PC::Help
{

class RmlLongNoticeLayer
{
public:
    struct State
    {
        bool visible = false;
        int viewportWidth = 0;
        int viewportHeight = 0;
        std::string text;
        float referenceX = 0.0f;
        float referenceY = 0.0f;
        float alphaRate = 0.0f;
        std::uint8_t textAlpha = 0;
        std::uint32_t textColor = 0;
        bool notice = false;
    };

    RmlLongNoticeLayer();
    ~RmlLongNoticeLayer();

    RmlLongNoticeLayer(const RmlLongNoticeLayer&) = delete;
    RmlLongNoticeLayer& operator=(const RmlLongNoticeLayer&) = delete;
    RmlLongNoticeLayer(RmlLongNoticeLayer&&) noexcept;
    RmlLongNoticeLayer& operator=(RmlLongNoticeLayer&&) noexcept;

    [[nodiscard]] bool Load(bool show = false);
    [[nodiscard]] bool Show();
    [[nodiscard]] bool Hide();
    void Release();

    [[nodiscard]] bool IsLoaded() const noexcept;
    [[nodiscard]] bool ApplyState(const State& state);
    [[nodiscard]] bool Update();

private:
    class Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace UI::Modern::PC::Help

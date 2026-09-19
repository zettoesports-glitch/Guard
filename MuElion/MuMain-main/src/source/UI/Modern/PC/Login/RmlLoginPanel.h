#pragma once

#include <memory>
#include <optional>
#include <string>

namespace UI::Modern::PC::Login
{

class RmlLoginPanel
{
public:
    enum class Action
    {
        Cancel,
        Menu,
        Credit,
    };

    struct LoginRequest
    {
        std::string account;
        std::string password;
    };

    RmlLoginPanel();
    ~RmlLoginPanel();

    RmlLoginPanel(const RmlLoginPanel&) = delete;
    RmlLoginPanel& operator=(const RmlLoginPanel&) = delete;
    RmlLoginPanel(RmlLoginPanel&&) noexcept;
    RmlLoginPanel& operator=(RmlLoginPanel&&) noexcept;

    [[nodiscard]] bool Load(bool show = false);
    [[nodiscard]] bool Show();
    [[nodiscard]] bool Hide();
    void Release();
    [[nodiscard]] bool IsLoaded() const noexcept;

    void SetViewport(int width, int height);
    void SetServerName(std::string name);
    void SetAccountLabel(std::string label);
    void SetPasswordLabel(std::string label);
    void SetButtonLabels(std::string ok, std::string cancel);

    void SetAccount(std::string account);
    void ClearPassword();

    [[nodiscard]] bool Update();

    [[nodiscard]] std::optional<LoginRequest> ConsumeLoginRequest();
    [[nodiscard]] std::optional<Action> ConsumeAction();

private:
    class Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace UI::Modern::PC::Login

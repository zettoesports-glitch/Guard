#include "stdafx.h"
#include "UI/Modern/PC/Login/RmlLoginPanel.h"

#include "UI/Modern/RmlDocumentHost.h"
#include "UI/Modern/RmlMuButton.h"
#include "UI/Modern/RmlUiDesign.h"

#include <RmlUi/Core/Core.h>
#include <RmlUi/Core/Element.h>
#include <RmlUi/Core/ElementDocument.h>
#include <RmlUi/Core/FileInterface.h>
#include <RmlUi/Core/StreamMemory.h>
#include <RmlUi/Core/StringUtilities.h>
#include <RmlUi/Core/Elements/ElementFormControlInput.h>

#include <algorithm>
#include <array>
#include <cstdio>
#include <sstream>
#include <utility>

namespace UI::Modern::PC::Login
{

namespace
{
constexpr const char* kLoginPath = "Data/UI/PC/Login/login.rml";
constexpr const char* kBottomPath = "Data/UI/PC/Login/login_bottom.rml";

RmlUiDesign::Values LoadDesignValues(const char* path)
{
    Rml::FileInterface* files = Rml::GetFileInterface();
    if (!files || !path)
        return {};

    Rml::String contents;
    if (!files->LoadFile(path, contents) || contents.empty())
        return {};

    Rml::StreamMemory stream(
        reinterpret_cast<const Rml::byte*>(contents.data()),
        contents.size());
    return RmlUiDesign::Parse(&stream);
}

template <typename T>
bool ParseScalar(const RmlUiDesign::Values& values,
                 const char* key, T& output)
{
    const auto it = values.find(key);
    if (it == values.end())
        return false;
    std::istringstream stream(it->second);
    T value{};
    if (!(stream >> value))
        return false;
    output = value;
    return true;
}

Rml::Element* ElementById(
    Rml::ElementDocument* document, const char* id)
{
    return document ? document->GetElementById(id) : nullptr;
}

Rml::ElementFormControlInput* InputById(
    Rml::ElementDocument* document, const char* id)
{
    return dynamic_cast<Rml::ElementFormControlInput*>(
        ElementById(document, id));
}

void SetEncodedText(Rml::Element* element, const std::string& text)
{
    if (element)
        element->SetInnerRML(Rml::StringUtilities::EncodeRml(text));
}

Rml::String Pixel(float value)
{
    std::array<char, 64> buffer{};
    std::snprintf(
        buffer.data(), buffer.size(), "%.3fpx",
        static_cast<double>(value));
    return buffer.data();
}
} // namespace

class RmlLoginPanel::Impl
{
public:
    struct LoginDesign
    {
        float width = 307.998413f;
        float height = 190.000717f;
        float anchorX = 0.5f;
        float anchorY = 0.666666667f;
    };

    struct BottomDesign
    {
        int columns = 2;
        float width = 70.0f;
        float height = 30.0f;
        float horizontalMargin = 30.0f;
        float bottomMargin = 11.0f;
        float baselineY = 0.945f;
    };

    Impl()
        : loginHost_(kLoginPath),
          bottomHost_(kBottomPath)
    {
    }

    ~Impl()
    {
        Release();
    }

    [[nodiscard]] bool Load(bool show)
    {
        if (loginHost_.IsLoaded())
        {
            if (show)
                return Show();
            return true;
        }

        if (!loginHost_.Load(false) || !bottomHost_.Load(false))
        {
            Release();
            return false;
        }

        loginDocument_ = loginHost_.GetDocument();
        bottomDocument_ = bottomHost_.GetDocument();
        if (!loginDocument_ || !bottomDocument_)
        {
            Release();
            return false;
        }

        ReadDesign();
        if (!Bind())
        {
            Release();
            return false;
        }

        ApplyLabels();
        ApplyLayout();
        visible_ = show;
        if (show)
            return loginHost_.Show() && bottomHost_.Show();
        return true;
    }

    [[nodiscard]] bool Show()
    {
        if (!loginHost_.IsLoaded() && !Load(false))
            return false;
        visible_ = true;
        return loginHost_.Show() && bottomHost_.Show();
    }

    [[nodiscard]] bool Hide()
    {
        visible_ = false;
        ClearPassword();
        const bool a = loginHost_.Hide();
        const bool b = bottomHost_.Hide();
        return a && b;
    }

    void Release()
    {
        okButton_.Unbind();
        cancelButton_.Unbind();
        menuButton_.Unbind();
        creditButton_.Unbind();

        loginRoot_ = nullptr;
        bottomRoot_ = nullptr;
        serverName_ = nullptr;
        accountLabel_ = nullptr;
        passwordLabel_ = nullptr;
        okLabel_ = nullptr;
        cancelLabel_ = nullptr;
        accountInput_ = nullptr;
        passwordInput_ = nullptr;
        loginDocument_ = nullptr;
        bottomDocument_ = nullptr;

        loginHost_.Release();
        bottomHost_.Release();

        pendingLogin_.reset();
        pendingAction_.reset();
    }

    [[nodiscard]] bool IsLoaded() const noexcept
    {
        return loginHost_.IsLoaded() &&
               bottomHost_.IsLoaded() &&
               loginDocument_ &&
               bottomDocument_;
    }

    void SetViewport(int width, int height)
    {
        viewportWidth_ = std::max(0, width);
        viewportHeight_ = std::max(0, height);
        ApplyLayout();
    }

    void SetServerName(std::string name)
    {
        serverNameText_ = std::move(name);
        SetEncodedText(serverName_, serverNameText_);
    }

    void SetAccountLabel(std::string label)
    {
        accountLabelText_ = std::move(label);
        SetEncodedText(accountLabel_, accountLabelText_);
    }

    void SetPasswordLabel(std::string label)
    {
        passwordLabelText_ = std::move(label);
        SetEncodedText(passwordLabel_, passwordLabelText_);
    }

    void SetButtonLabels(std::string ok, std::string cancel)
    {
        okText_ = std::move(ok);
        cancelText_ = std::move(cancel);
        SetEncodedText(okLabel_, okText_);
        SetEncodedText(cancelLabel_, cancelText_);
    }

    void SetAccount(std::string account)
    {
        accountText_ = std::move(account);
        if (accountInput_)
            accountInput_->SetValue(accountText_);
    }

    void ClearPassword()
    {
        if (passwordInput_)
            passwordInput_->SetValue("");
    }

    [[nodiscard]] bool Update()
    {
        if (!IsLoaded())
            return false;

        bool changed = false;
        changed |= okButton_.Update();
        changed |= cancelButton_.Update();
        changed |= menuButton_.Update();
        changed |= creditButton_.Update();

        if (okButton_.ConsumeClicked())
        {
            LoginRequest request;
            request.account =
                accountInput_ ? accountInput_->GetValue() : std::string{};
            request.password =
                passwordInput_ ? passwordInput_->GetValue() : std::string{};

            if (!request.account.empty() && !request.password.empty())
            {
                accountText_ = request.account;
                pendingLogin_ = std::move(request);
            }
            changed = true;
        }

        if (cancelButton_.ConsumeClicked())
        {
            pendingAction_ = Action::Cancel;
            ClearPassword();
            changed = true;
        }
        if (menuButton_.ConsumeClicked())
        {
            pendingAction_ = Action::Menu;
            changed = true;
        }
        if (creditButton_.ConsumeClicked())
        {
            pendingAction_ = Action::Credit;
            changed = true;
        }

        return changed || pendingLogin_.has_value() || pendingAction_.has_value();
    }

    [[nodiscard]] std::optional<LoginRequest> ConsumeLoginRequest()
    {
        auto result = std::exchange(pendingLogin_, std::nullopt);
        ClearPassword();
        return result;
    }

    [[nodiscard]] std::optional<Action> ConsumeAction()
    {
        return std::exchange(pendingAction_, std::nullopt);
    }

private:
    void ReadDesign()
    {
        const auto login = LoadDesignValues(kLoginPath);
        (void)ParseScalar(login, "Login-Width", loginDesign_.width);
        (void)ParseScalar(login, "Login-Height", loginDesign_.height);
        (void)ParseScalar(login, "Login-AnchorX", loginDesign_.anchorX);
        (void)ParseScalar(login, "Login-AnchorY", loginDesign_.anchorY);

        const auto bottom = LoadDesignValues(kBottomPath);
        (void)ParseScalar(bottom, "LoginBottom-Columns", bottomDesign_.columns);
        (void)ParseScalar(bottom, "LoginBottom-Width", bottomDesign_.width);
        (void)ParseScalar(bottom, "LoginBottom-Height", bottomDesign_.height);
        (void)ParseScalar(
            bottom, "LoginBottom-HorizontalMargin",
            bottomDesign_.horizontalMargin);
        (void)ParseScalar(
            bottom, "LoginBottom-BottomMargin",
            bottomDesign_.bottomMargin);
        (void)ParseScalar(
            bottom, "LoginBottom-BaselineY",
            bottomDesign_.baselineY);
    }

    [[nodiscard]] bool Bind()
    {
        loginRoot_ = ElementById(loginDocument_, "login-panel");
        bottomRoot_ = ElementById(bottomDocument_, "login-bottom-root");
        serverName_ = ElementById(loginDocument_, "server-name");
        accountLabel_ = ElementById(loginDocument_, "account-label");
        passwordLabel_ = ElementById(loginDocument_, "password-label");
        okLabel_ = ElementById(loginDocument_, "ok-label");
        cancelLabel_ = ElementById(loginDocument_, "cancel-label");
        accountInput_ = InputById(loginDocument_, "account-input");
        passwordInput_ = InputById(loginDocument_, "password-input");

        Rml::Element* ok = ElementById(loginDocument_, "ok-button");
        Rml::Element* cancel = ElementById(loginDocument_, "cancel-button");
        Rml::Element* menu = ElementById(bottomDocument_, "menu-button");
        Rml::Element* credit = ElementById(bottomDocument_, "credit-button");

        if (!loginRoot_ || !bottomRoot_ || !serverName_ ||
            !accountLabel_ || !passwordLabel_ || !okLabel_ || !cancelLabel_ ||
            !accountInput_ || !passwordInput_ ||
            !ok || !cancel || !menu || !credit)
            return false;

        okButton_.Bind(ok);
        cancelButton_.Bind(cancel);
        menuButton_.Bind(menu);
        creditButton_.Bind(credit);

        if (!accountText_.empty())
            accountInput_->SetValue(accountText_);
        return true;
    }

    void ApplyLabels()
    {
        SetEncodedText(serverName_, serverNameText_);
        SetEncodedText(accountLabel_, accountLabelText_);
        SetEncodedText(passwordLabel_, passwordLabelText_);
        SetEncodedText(okLabel_, okText_);
        SetEncodedText(cancelLabel_, cancelText_);
    }

    void ApplyLayout()
    {
        if (!loginRoot_ || !bottomRoot_ ||
            viewportWidth_ <= 0 || viewportHeight_ <= 0)
            return;

        const float left =
            static_cast<float>(viewportWidth_) * loginDesign_.anchorX -
            loginDesign_.width * 0.5f;
        const float top =
            static_cast<float>(viewportHeight_) * loginDesign_.anchorY -
            loginDesign_.height * 0.5f;

        loginRoot_->SetProperty("left", Pixel(left));
        loginRoot_->SetProperty("top", Pixel(top));
        loginRoot_->SetProperty("width", Pixel(loginDesign_.width));
        loginRoot_->SetProperty("height", Pixel(loginDesign_.height));

        const int columns = std::max(1, bottomDesign_.columns);
        const float totalWidth =
            columns * bottomDesign_.width +
            (columns - 1) * bottomDesign_.horizontalMargin;
        const float bottomLeft =
            (static_cast<float>(viewportWidth_) - totalWidth) * 0.5f;
        const float baseline =
            static_cast<float>(viewportHeight_) * bottomDesign_.baselineY;
        const float bottomTop =
            baseline - bottomDesign_.height - bottomDesign_.bottomMargin;

        bottomRoot_->SetProperty("left", Pixel(bottomLeft));
        bottomRoot_->SetProperty("top", Pixel(bottomTop));
        bottomRoot_->SetProperty("width", Pixel(totalWidth));
        bottomRoot_->SetProperty("height", Pixel(bottomDesign_.height));

        if (Rml::Element* menu = ElementById(bottomDocument_, "menu-button"))
        {
            menu->SetProperty("left", "0px");
            menu->SetProperty("width", Pixel(bottomDesign_.width));
            menu->SetProperty("height", Pixel(bottomDesign_.height));
        }

        if (Rml::Element* credit = ElementById(bottomDocument_, "credit-button"))
        {
            credit->SetProperty(
                "left",
                Pixel(bottomDesign_.width + bottomDesign_.horizontalMargin));
            credit->SetProperty("width", Pixel(bottomDesign_.width));
            credit->SetProperty("height", Pixel(bottomDesign_.height));
        }
    }

    RmlDocumentHost loginHost_;
    RmlDocumentHost bottomHost_;
    Rml::ElementDocument* loginDocument_ = nullptr;
    Rml::ElementDocument* bottomDocument_ = nullptr;

    Rml::Element* loginRoot_ = nullptr;
    Rml::Element* bottomRoot_ = nullptr;
    Rml::Element* serverName_ = nullptr;
    Rml::Element* accountLabel_ = nullptr;
    Rml::Element* passwordLabel_ = nullptr;
    Rml::Element* okLabel_ = nullptr;
    Rml::Element* cancelLabel_ = nullptr;
    Rml::ElementFormControlInput* accountInput_ = nullptr;
    Rml::ElementFormControlInput* passwordInput_ = nullptr;

    RmlMuButton okButton_;
    RmlMuButton cancelButton_;
    RmlMuButton menuButton_;
    RmlMuButton creditButton_;

    LoginDesign loginDesign_;
    BottomDesign bottomDesign_;
    int viewportWidth_ = 0;
    int viewportHeight_ = 0;
    bool visible_ = false;

    std::string serverNameText_;
    std::string accountLabelText_ = "Account";
    std::string passwordLabelText_ = "Password";
    std::string okText_ = "OK";
    std::string cancelText_ = "Cancel";
    std::string accountText_;

    std::optional<LoginRequest> pendingLogin_;
    std::optional<Action> pendingAction_;
};

RmlLoginPanel::RmlLoginPanel()
    : m_impl(std::make_unique<Impl>())
{
}

RmlLoginPanel::~RmlLoginPanel() = default;
RmlLoginPanel::RmlLoginPanel(RmlLoginPanel&&) noexcept = default;
RmlLoginPanel& RmlLoginPanel::operator=(RmlLoginPanel&&) noexcept = default;

bool RmlLoginPanel::Load(bool show)
{
    return m_impl && m_impl->Load(show);
}

bool RmlLoginPanel::Show()
{
    return m_impl && m_impl->Show();
}

bool RmlLoginPanel::Hide()
{
    return m_impl && m_impl->Hide();
}

void RmlLoginPanel::Release()
{
    if (m_impl)
        m_impl->Release();
}

bool RmlLoginPanel::IsLoaded() const noexcept
{
    return m_impl && m_impl->IsLoaded();
}

void RmlLoginPanel::SetViewport(int width, int height)
{
    if (m_impl)
        m_impl->SetViewport(width, height);
}

void RmlLoginPanel::SetServerName(std::string name)
{
    if (m_impl)
        m_impl->SetServerName(std::move(name));
}

void RmlLoginPanel::SetAccountLabel(std::string label)
{
    if (m_impl)
        m_impl->SetAccountLabel(std::move(label));
}

void RmlLoginPanel::SetPasswordLabel(std::string label)
{
    if (m_impl)
        m_impl->SetPasswordLabel(std::move(label));
}

void RmlLoginPanel::SetButtonLabels(std::string ok, std::string cancel)
{
    if (m_impl)
        m_impl->SetButtonLabels(std::move(ok), std::move(cancel));
}

void RmlLoginPanel::SetAccount(std::string account)
{
    if (m_impl)
        m_impl->SetAccount(std::move(account));
}

void RmlLoginPanel::ClearPassword()
{
    if (m_impl)
        m_impl->ClearPassword();
}

bool RmlLoginPanel::Update()
{
    return m_impl && m_impl->Update();
}

std::optional<RmlLoginPanel::LoginRequest>
RmlLoginPanel::ConsumeLoginRequest()
{
    return m_impl ? m_impl->ConsumeLoginRequest() : std::nullopt;
}

std::optional<RmlLoginPanel::Action>
RmlLoginPanel::ConsumeAction()
{
    return m_impl ? m_impl->ConsumeAction() : std::nullopt;
}

} // namespace UI::Modern::PC::Login

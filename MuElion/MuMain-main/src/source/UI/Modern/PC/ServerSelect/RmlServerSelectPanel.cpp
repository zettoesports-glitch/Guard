#include "stdafx.h"
#include "UI/Modern/PC/ServerSelect/RmlServerSelectPanel.h"

#include "UI/Modern/RmlDocumentHost.h"
#include "UI/Modern/RmlMuButton.h"

#include <RmlUi/Core/Element.h>
#include <RmlUi/Core/ElementDocument.h>

#include <algorithm>
#include <array>
#include <cstdio>
#include <string>
#include <utility>

namespace UI::Modern::PC::ServerSelect
{

namespace
{
constexpr const char* kDocumentPath =
    "Data/UI/PC/ServerSelect/server_select.rml";

Rml::String PixelValue(float value)
{
    std::array<char, 64> buffer{};
    std::snprintf(buffer.data(), buffer.size(), "%.3fpx",
                  static_cast<double>(value));
    return Rml::String(buffer.data());
}

Rml::String PercentValue(int value)
{
    value = std::clamp(value, 0, 100);
    return std::to_string(value) + "%";
}

Rml::Element* RequiredElement(
    Rml::ElementDocument* document, const char* id)
{
    return document ? document->GetElementById(id) : nullptr;
}

const char* KindClass(
    RmlServerSelectPanel::ServerKind kind) noexcept
{
    switch (kind)
    {
    case RmlServerSelectPanel::ServerKind::NonPvp: return "non-pvp";
    case RmlServerSelectPanel::ServerKind::GoldPvp:
    case RmlServerSelectPanel::ServerKind::Gold: return "gold";
    case RmlServerSelectPanel::ServerKind::Pvp:
    default: return "pvp";
    }
}
} // namespace

class RmlServerSelectPanel::Impl
{
public:
    Impl() : host_(kDocumentPath) {}
    ~Impl() { Release(); }

    [[nodiscard]] bool Load(bool show)
    {
        if (host_.IsLoaded())
        {
            if (show)
                return Show();
            return true;
        }

        if (!host_.Load(show))
            return false;

        document_ = host_.GetDocument();
        if (!document_ || !Bind())
        {
            Release();
            return false;
        }

        visible_ = show;
        dirty_ = true;
        return Refresh();
    }

    [[nodiscard]] bool Show()
    {
        if (!host_.IsLoaded() && !Load(false))
            return false;
        visible_ = true;
        dirty_ = true;
        return host_.Show();
    }

    [[nodiscard]] bool Hide()
    {
        visible_ = false;
        return host_.Hide();
    }

    void Release()
    {
        groupButtons_.clear();
        serverButtons_.clear();

        root_ = nullptr;
        title_ = nullptr;
        groupList_ = nullptr;
        serverList_ = nullptr;
        description_ = nullptr;
        advisory_ = nullptr;
        document_ = nullptr;

        host_.Release();
        pendingSelection_.reset();
        dirty_ = false;
    }

    [[nodiscard]] bool IsLoaded() const noexcept
    {
        return host_.IsLoaded() && document_ != nullptr;
    }

    void SetViewport(int width, int height) noexcept
    {
        viewportWidth_ = std::max(0, width);
        viewportHeight_ = std::max(0, height);
        dirty_ = true;
    }

    [[nodiscard]] bool ApplyState(State state)
    {
        state_ = std::move(state);

        if (state_.groups.empty())
            state_.selectedGroupKey = -1;
        else if (!FindGroup(state_.selectedGroupKey))
            state_.selectedGroupKey = state_.groups.front().key;

        dirty_ = true;
        return !IsLoaded() || Refresh();
    }

    [[nodiscard]] bool Update()
    {
        if (!IsLoaded())
            return false;

        bool changed = false;

        for (std::size_t i = 0; i < groupButtons_.size(); ++i)
        {
            RmlMuButton& button = *groupButtons_[i].button;
            changed |= button.Update();
            if (button.ConsumeClicked() && i < state_.groups.size())
            {
                state_.selectedGroupKey = state_.groups[i].key;
                dirty_ = true;
                changed = true;
            }
        }

        const Group* group = FindGroup(state_.selectedGroupKey);
        for (std::size_t i = 0; i < serverButtons_.size(); ++i)
        {
            RmlMuButton& button = *serverButtons_[i].button;
            changed |= button.Update();
            if (!button.ConsumeClicked() || !group ||
                i >= group->servers.size())
                continue;

            const Server& server = group->servers[i];
            if (!server.enabled)
                continue;

            pendingSelection_ = Selection{
                group->key,
                server.connectIndex,
                server.serverIndex,
                server.kind,
                server.name,
            };
            changed = true;
        }

        if (dirty_)
        {
            changed |= Refresh();
            dirty_ = false;
        }

        return changed;
    }

    [[nodiscard]] std::optional<Selection> ConsumeSelection()
    {
        return std::exchange(pendingSelection_, std::nullopt);
    }

private:
    struct ButtonBinding
    {
        std::unique_ptr<RmlMuButton> button;
        Rml::Element* element = nullptr;
    };

    [[nodiscard]] bool Bind()
    {
        root_ = RequiredElement(document_, "server-select");
        title_ = RequiredElement(document_, "title");
        groupList_ = RequiredElement(document_, "group-list");
        serverList_ = RequiredElement(document_, "server-list");
        description_ = RequiredElement(document_, "description");
        advisory_ = RequiredElement(document_, "advisory");

        return root_ && title_ && groupList_ && serverList_ &&
               description_ && advisory_;
    }

    [[nodiscard]] const Group* FindGroup(int key) const noexcept
    {
        const auto it = std::find_if(
            state_.groups.begin(), state_.groups.end(),
            [key](const Group& group) { return group.key == key; });
        return it == state_.groups.end() ? nullptr : &*it;
    }

    [[nodiscard]] bool Refresh()
    {
        if (!IsLoaded())
            return false;

        PositionRoot();
        title_->SetInnerRML(state_.title);
        advisory_->SetInnerRML(state_.advisory);

        BuildGroups();
        BuildServers();

        const Group* selected = FindGroup(state_.selectedGroupKey);
        description_->SetInnerRML(
            selected ? selected->description : Rml::String{});

        document_->UpdateDocument();
        return true;
    }

    void PositionRoot()
    {
        constexpr float width = 500.0f;
        constexpr float height = 460.0f;

        const float left = std::max(
            0.0f, (static_cast<float>(viewportWidth_) - width) * 0.5f);
        const float top = std::max(
            0.0f, (static_cast<float>(viewportHeight_) - height) * 0.5f);

        root_->SetProperty("left", PixelValue(left));
        root_->SetProperty("top", PixelValue(top));
    }

    void BuildGroups()
    {
        groupButtons_.clear();
        groupList_->SetInnerRML("");

        for (std::size_t index = 0; index < state_.groups.size(); ++index)
        {
            const Group& group = state_.groups[index];

            Rml::ElementPtr row = document_->CreateElement("div");
            if (!row)
                continue;

            row->SetId(
                Rml::String("server-group-") + std::to_string(index));
            row->SetClass("server-group", true);
            row->SetClass(
                "selected", group.key == state_.selectedGroupKey);
            row->AppendChild(document_->CreateTextNode(group.name));

            Rml::Element* raw =
                groupList_->AppendChild(std::move(row));
            if (!raw)
                continue;

            auto button = std::make_unique<RmlMuButton>();
            button->Bind(raw);
            groupButtons_.push_back(
                {std::move(button), raw});
        }
    }

    void BuildServers()
    {
        serverButtons_.clear();
        serverList_->SetInnerRML("");

        const Group* group = FindGroup(state_.selectedGroupKey);
        if (!group)
            return;

        for (std::size_t index = 0; index < group->servers.size(); ++index)
        {
            const Server& server = group->servers[index];

            Rml::ElementPtr row = document_->CreateElement("div");
            if (!row)
                continue;

            row->SetId(
                Rml::String("server-row-") + std::to_string(index));
            row->SetClass("server-row", true);
            row->SetClass(KindClass(server.kind), true);
            row->SetClass("busy", server.loadPercent >= 100);
            row->SetClass("offline", !server.enabled);

            Rml::ElementPtr name = document_->CreateElement("div");
            name->SetClass("server-name", true);
            name->AppendChild(document_->CreateTextNode(server.name));
            row->AppendChild(std::move(name));

            Rml::ElementPtr load = document_->CreateElement("div");
            load->SetClass("server-load", true);

            Rml::ElementPtr fill = document_->CreateElement("div");
            fill->SetClass("server-load-fill", true);
            fill->SetProperty(
                "width", PercentValue(server.loadPercent));
            load->AppendChild(std::move(fill));
            row->AppendChild(std::move(load));

            Rml::Element* raw =
                serverList_->AppendChild(std::move(row));
            if (!raw)
                continue;

            auto button = std::make_unique<RmlMuButton>();
            button->Bind(raw);
            button->SetEnabled(server.enabled);
            (void)button->Update();
            serverButtons_.push_back(
                {std::move(button), raw});
        }
    }

    RmlDocumentHost host_;
    Rml::ElementDocument* document_ = nullptr;
    Rml::Element* root_ = nullptr;
    Rml::Element* title_ = nullptr;
    Rml::Element* groupList_ = nullptr;
    Rml::Element* serverList_ = nullptr;
    Rml::Element* description_ = nullptr;
    Rml::Element* advisory_ = nullptr;

    std::vector<ButtonBinding> groupButtons_;
    std::vector<ButtonBinding> serverButtons_;

    State state_;
    int viewportWidth_ = 0;
    int viewportHeight_ = 0;
    bool visible_ = false;
    bool dirty_ = false;
    std::optional<Selection> pendingSelection_;
};

RmlServerSelectPanel::RmlServerSelectPanel()
    : m_impl(std::make_unique<Impl>())
{
}
RmlServerSelectPanel::~RmlServerSelectPanel() = default;
RmlServerSelectPanel::RmlServerSelectPanel(
    RmlServerSelectPanel&&) noexcept = default;
RmlServerSelectPanel& RmlServerSelectPanel::operator=(
    RmlServerSelectPanel&&) noexcept = default;

bool RmlServerSelectPanel::Load(bool show)
{
    return m_impl && m_impl->Load(show);
}
bool RmlServerSelectPanel::Show()
{
    return m_impl && m_impl->Show();
}
bool RmlServerSelectPanel::Hide()
{
    return m_impl && m_impl->Hide();
}
void RmlServerSelectPanel::Release()
{
    if (m_impl)
        m_impl->Release();
}
bool RmlServerSelectPanel::IsLoaded() const noexcept
{
    return m_impl && m_impl->IsLoaded();
}
void RmlServerSelectPanel::SetViewport(int width, int height) noexcept
{
    if (m_impl)
        m_impl->SetViewport(width, height);
}
bool RmlServerSelectPanel::ApplyState(State state)
{
    return m_impl && m_impl->ApplyState(std::move(state));
}
bool RmlServerSelectPanel::Update()
{
    return m_impl && m_impl->Update();
}
std::optional<RmlServerSelectPanel::Selection>
RmlServerSelectPanel::ConsumeSelection()
{
    return m_impl ? m_impl->ConsumeSelection() : std::nullopt;
}

} // namespace UI::Modern::PC::ServerSelect

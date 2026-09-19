#include "stdafx.h"
#include "UI/Modern/PC/Inventory/RmlTradePanel.h"

#include "UI/Modern/RmlDocumentHost.h"
#include "UI/Modern/RmlMuButton.h"
#include "UI/Modern/RmlMuMovablePanel.h"
#include "UI/Modern/RmlMuSlot.h"
#include "UI/Modern/RmlUiDesign.h"

#include <RmlUi/Core/Core.h>
#include <RmlUi/Core/Element.h>
#include <RmlUi/Core/ElementDocument.h>
#include <RmlUi/Core/FileInterface.h>
#include <RmlUi/Core/StreamMemory.h>

#include <algorithm>
#include <array>
#include <cstdio>
#include <sstream>
#include <utility>

namespace UI::Modern::PC::Inventory
{

namespace
{
constexpr const char* kTradePath =
    "Data/UI/PC/Inventory/trade.rml";
constexpr const char* kConfirmPath =
    "Data/UI/PC/Inventory/trade_confirm.rml";

struct Design
{
    float width = 327.0f;
    float height = 639.0f;
    float otherX = 60.0f;
    float otherY = 163.0f;
    float pitchX = 26.0f;
    float pitchY = 26.0f;
    float myX = 60.0f;
    float myY = 410.0f;
    float initialX = 21.0f;
    float initialY = 40.0f;
};

Rml::String Px(float value)
{
    std::array<char, 64> buffer{};
    std::snprintf(buffer.data(), buffer.size(), "%.3fpx",
                  static_cast<double>(value));
    return Rml::String(buffer.data());
}

Rml::String Scale(float value)
{
    std::array<char, 64> buffer{};
    std::snprintf(buffer.data(), buffer.size(), "scale(%.6f)",
                  static_cast<double>(value));
    return Rml::String(buffer.data());
}

RmlUiDesign::Values DesignValues()
{
    Rml::FileInterface* files = Rml::GetFileInterface();
    if (!files)
        return {};
    Rml::String contents;
    if (!files->LoadFile(kTradePath, contents) || contents.empty())
        return {};
    Rml::StreamMemory stream(
        reinterpret_cast<const Rml::byte*>(contents.data()), contents.size());
    return RmlUiDesign::Parse(&stream);
}

void ReadDesign(Design& d)
{
    const auto values = DesignValues();
    if (const auto it=values.find("Panel-Size"); it!=values.end())
    {
        std::istringstream s(it->second);
        (void)(s>>d.width>>d.height);
    }
    if (const auto it=values.find("Panel-Grids"); it!=values.end())
    {
        std::istringstream s(it->second);
        (void)(s>>d.otherX>>d.otherY>>d.pitchX>>d.pitchY
                >>d.myX>>d.myY>>d.pitchX>>d.pitchY);
    }
    if (const auto it=values.find("Panel-Initial"); it!=values.end())
    {
        std::istringstream s(it->second);
        (void)(s>>d.initialX>>d.initialY);
    }
}

Rml::Element* E(Rml::ElementDocument* doc, const Rml::String& id)
{
    return doc ? doc->GetElementById(id) : nullptr;
}

void Text(Rml::Element* e, const std::string& value)
{
    if (e) e->SetInnerRML(value);
}
} // namespace

class RmlTradePanel::Impl
{
public:
    Impl() : tradeHost_(kTradePath), confirmHost_(kConfirmPath) {}
    ~Impl() { Release(); }

    bool Load(bool show)
    {
        if (loaded_)
            return show ? Show() : true;

        if (!tradeHost_.Load(show) || !confirmHost_.Load(false))
        {
            Release();
            return false;
        }

        tradeDoc_=tradeHost_.GetDocument();
        confirmDoc_=confirmHost_.GetDocument();
        if (!tradeDoc_ || !confirmDoc_)
        {
            Release();
            return false;
        }

        ReadDesign(design_);
        if (!Bind() || !CreateSlots())
        {
            Release();
            return false;
        }

        loaded_=true;
        if (show) state_.visible=true;
        return ApplyState(state_);
    }

    bool Show()
    {
        if (!Load(false)) return false;
        state_.visible=true;
        const bool result=tradeHost_.Show();
        if (result) (void)ApplyState(state_);
        return result;
    }

    bool Hide()
    {
        state_.visible=false;
        (void)confirmHost_.Hide();
        return tradeHost_.Hide();
    }

    void Release()
    {
        mover_.Unbind();
        close_.Unbind();
        inputZen_.Unbind();
        confirm_.Unbind();
        for(auto& s: otherSlots_) s.Unbind();
        for(auto& s: mySlots_) s.Unbind();
        otherElements_.fill(nullptr);
        myElements_.fill(nullptr);
        tradeDoc_=nullptr;
        confirmDoc_=nullptr;
        root_=drag_=nullptr;
        loaded_=false;
        pending_.reset();
        confirmHost_.Release();
        tradeHost_.Release();
    }

    bool IsLoaded() const noexcept { return loaded_; }

    bool ApplyState(const State& state)
    {
        state_=state;
        if (!loaded_ && !Load(false))
            return false;

        const bool visible=state_.visible &&
            state_.viewportWidth>0 && state_.viewportHeight>0;
        root_->SetProperty("display", visible?"block":"none");
        if (!visible)
        {
            (void)confirmHost_.Hide();
            return true;
        }

        ApplyLayout();

        Text(E(tradeDoc_,"tfTitle"),state_.title);
        Text(E(tradeDoc_,"tfTradeOtherValue"),state_.otherName);
        Text(E(tradeDoc_,"tfGuildValue"),state_.otherGuild);
        Text(E(tradeDoc_,"tfLevelValue"),state_.otherLevel);
        Text(E(tradeDoc_,"tfOtherZen"),state_.otherZen);
        Text(E(tradeDoc_,"tfMyNameValue"),state_.myName);
        Text(E(tradeDoc_,"tfMyZen"),state_.myZen);
        Text(E(tradeDoc_,"tfCautionText"),state_.cautionText);

        confirm_.SetEnabled(state_.confirmEnabled);
        if (confirmElement_)
        {
            confirmElement_->SetClass("selected",state_.myConfirmed);
            confirmElement_->SetClass("alert",state_.tradeAlert);
        }

        if (state_.myConfirmed || state_.otherConfirmed)
        {
            Text(E(confirmDoc_,"tfMyConfirm"),
                 state_.myConfirmed?"Confirmed":"");
            Text(E(confirmDoc_,"tfOtherConfirm"),
                 state_.otherConfirmed?"Confirmed":"");
            (void)confirmHost_.Show();
        }
        else
        {
            (void)confirmHost_.Hide();
        }

        for(std::size_t i=0;i<kSlotsPerSide;++i)
        {
            ApplySlot(otherSlots_[i],otherElements_[i],state_.otherSlots[i]);
            ApplySlot(mySlots_[i],myElements_[i],state_.mySlots[i]);
        }

        (void)close_.Update();
        (void)inputZen_.Update();
        (void)confirm_.Update();
        tradeDoc_->UpdateDocument();
        confirmDoc_->UpdateDocument();
        return true;
    }

    bool Update()
    {
        if(!loaded_) return false;
        bool changed=close_.Update()|inputZen_.Update()|confirm_.Update();

        if(close_.ConsumeClicked())
        { pending_=Action{ActionType::Close,0}; changed=true; }
        if(inputZen_.ConsumeClicked())
        { pending_=Action{ActionType::InputZen,0}; changed=true; }
        if(confirm_.ConsumeClicked())
        { pending_=Action{ActionType::ToggleConfirm,0}; changed=true; }

        for(std::size_t i=0;i<kSlotsPerSide;++i)
        {
            changed|=otherSlots_[i].Update();
            changed|=mySlots_[i].Update();

            if(otherSlots_[i].ConsumeClicked() ||
               otherSlots_[i].ConsumeSecondaryClicked())
            { pending_=Action{ActionType::OtherSlotInspect,i}; changed=true; }

            if(mySlots_[i].ConsumeClicked())
            { pending_=Action{ActionType::MySlotPrimary,i}; changed=true; }
            if(mySlots_[i].ConsumeSecondaryClicked())
            { pending_=Action{ActionType::MySlotSecondary,i}; changed=true; }
        }

        changed|=mover_.ConsumePositionChanged();
        return changed;
    }

    std::optional<Action> ConsumeAction()
    { return std::exchange(pending_,std::nullopt); }

private:
    bool Bind()
    {
        root_=E(tradeDoc_,"panel");
        drag_=E(tradeDoc_,"btnDrag");
        Rml::Element* close=E(tradeDoc_,"btnClose");
        Rml::Element* zen=E(tradeDoc_,"btnInputZen");
        confirmElement_=E(tradeDoc_,"btnTrade");
        otherGrid_=E(tradeDoc_,"trade-other-grid");
        myGrid_=E(tradeDoc_,"trade-my-grid");
        if(!root_||!drag_||!close||!zen||!confirmElement_||
           !otherGrid_||!myGrid_) return false;
        close_.Bind(close);
        inputZen_.Bind(zen);
        confirm_.Bind(confirmElement_);
        mover_.Bind(root_,drag_);
        return true;
    }

    bool CreateSlots()
    {
        otherGrid_->SetInnerRML("");
        myGrid_->SetInnerRML("");

        for(std::size_t i=0;i<kSlotsPerSide;++i)
        {
            auto other=tradeDoc_->CreateElement("div");
            auto mine=tradeDoc_->CreateElement("div");
            if(!other||!mine) return false;

            other->SetId(Rml::String("isSlot")+std::to_string(i));
            other->SetClass("mu-item-slot",true);
            other->SetClass("trade-slot",true);
            other->SetClass("other",true);

            mine->SetId(Rml::String("isSlot")+std::to_string(i+kSlotsPerSide));
            mine->SetClass("mu-item-slot",true);
            mine->SetClass("trade-slot",true);
            mine->SetClass("mine",true);

            Rml::Element* o=otherGrid_->AppendChild(std::move(other));
            Rml::Element* m=myGrid_->AppendChild(std::move(mine));
            if(!o||!m) return false;
            otherElements_[i]=o;
            myElements_[i]=m;
            otherSlots_[i].Bind(o);
            mySlots_[i].Bind(m);
        }
        return true;
    }

    static void ApplySlot(RmlMuSlot& slot,Rml::Element* e,const SlotState& s)
    {
        slot.SetVisible(s.visible);
        slot.SetEnabled(s.enabled);
        slot.SetIconFrame(s.iconFrame);
        if(e)
        {
            e->SetClass("occupied",s.occupied);
            e->SetClass("selected",s.selected);
            e->SetClass("warning",s.warning);
        }
        (void)slot.Update();
    }

    void ApplyLayout()
    {
        const float vw=static_cast<float>(state_.viewportWidth);
        const float vh=static_cast<float>(state_.viewportHeight);
        const float scale=std::clamp(
            std::min(vw/std::max(1.0f,design_.initialX+design_.width),
                     vh/std::max(1.0f,design_.initialY+design_.height)),
            0.25f,1.0f);

        root_->SetProperty("width",Px(design_.width));
        root_->SetProperty("height",Px(design_.height));
        root_->SetProperty("transform-origin","0 0");
        root_->SetProperty("transform",Scale(scale));
        mover_.SetMetrics(vw/scale,vh/scale,design_.width,design_.height,0,0);
        const auto pos=mover_.GetPosition();
        if(pos.x==0&&pos.y==0) mover_.SetPosition(design_.initialX,design_.initialY);

        LayoutGrid(otherGrid_,design_.otherX,design_.otherY,otherElements_);
        LayoutGrid(myGrid_,design_.myX,design_.myY,myElements_);

        if(Rml::Element* confirmRoot=E(confirmDoc_,"trade-confirm"))
        {
            confirmRoot->SetProperty("left",Px(design_.initialX));
            confirmRoot->SetProperty("top",Px(design_.initialY));
            confirmRoot->SetProperty("width",Px(design_.width));
            confirmRoot->SetProperty("height",Px(design_.height));
            confirmRoot->SetProperty("transform-origin","0 0");
            confirmRoot->SetProperty("transform",Scale(scale));
        }
    }

    void LayoutGrid(Rml::Element* grid,float x,float y,
                    const std::array<Rml::Element*,kSlotsPerSide>& elements)
    {
        grid->SetProperty("left",Px(x));
        grid->SetProperty("top",Px(y));
        for(std::size_t i=0;i<elements.size();++i)
        {
            auto* e=elements[i];
            e->SetProperty("left",Px((i%kColumns)*design_.pitchX));
            e->SetProperty("top",Px((i/kColumns)*design_.pitchY));
            e->SetProperty("width",Px(design_.pitchX));
            e->SetProperty("height",Px(design_.pitchY));
        }
    }

    RmlDocumentHost tradeHost_;
    RmlDocumentHost confirmHost_;
    Rml::ElementDocument* tradeDoc_=nullptr;
    Rml::ElementDocument* confirmDoc_=nullptr;
    Rml::Element* root_=nullptr;
    Rml::Element* drag_=nullptr;
    Rml::Element* otherGrid_=nullptr;
    Rml::Element* myGrid_=nullptr;
    Rml::Element* confirmElement_=nullptr;

    std::array<Rml::Element*,kSlotsPerSide> otherElements_{};
    std::array<Rml::Element*,kSlotsPerSide> myElements_{};
    std::array<RmlMuSlot,kSlotsPerSide> otherSlots_{};
    std::array<RmlMuSlot,kSlotsPerSide> mySlots_{};
    RmlMuButton close_;
    RmlMuButton inputZen_;
    RmlMuButton confirm_;
    RmlMuMovablePanel mover_;
    Design design_;
    State state_;
    std::optional<Action> pending_;
    bool loaded_=false;
};

RmlTradePanel::RmlTradePanel():m_impl(std::make_unique<Impl>()){}
RmlTradePanel::~RmlTradePanel()=default;
RmlTradePanel::RmlTradePanel(RmlTradePanel&&) noexcept=default;
RmlTradePanel& RmlTradePanel::operator=(RmlTradePanel&&) noexcept=default;
bool RmlTradePanel::Load(bool show){return m_impl&&m_impl->Load(show);}
bool RmlTradePanel::Show(){return m_impl&&m_impl->Show();}
bool RmlTradePanel::Hide(){return m_impl&&m_impl->Hide();}
void RmlTradePanel::Release(){if(m_impl)m_impl->Release();}
bool RmlTradePanel::IsLoaded()const noexcept{return m_impl&&m_impl->IsLoaded();}
bool RmlTradePanel::ApplyState(const State& state){return m_impl&&m_impl->ApplyState(state);}
bool RmlTradePanel::Update(){return m_impl&&m_impl->Update();}
std::optional<RmlTradePanel::Action> RmlTradePanel::ConsumeAction()
{return m_impl?m_impl->ConsumeAction():std::nullopt;}

} // namespace UI::Modern::PC::Inventory

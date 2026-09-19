#include "stdafx.h"
#include "UI/Modern/PC/Combat/RmlDuelWatchPanel.h"
#include "UI/Modern/RmlDocumentHost.h"
#include "UI/Modern/RmlMuButton.h"
#include "UI/Modern/RmlMuMovablePanel.h"
#include "UI/Modern/RmlUiDesign.h"

#include <RmlUi/Core/Core.h>
#include <RmlUi/Core/Element.h>
#include <RmlUi/Core/ElementDocument.h>
#include <RmlUi/Core/FileInterface.h>
#include <RmlUi/Core/StreamMemory.h>

#include <array>
#include <cstdio>
#include <sstream>
#include <utility>

namespace UI::Modern::PC::Combat
{
namespace
{
constexpr const char* kDocumentPath="Data/UI/PC/Combat/duel_watch.rml";

Rml::String PixelValue(float value)
{
    std::array<char,64> b{};
    std::snprintf(b.data(),b.size(),"%.3fpx",static_cast<double>(value));
    return Rml::String(b.data());
}
bool ParsePair(const RmlUiDesign::Values& values,const char* key,std::array<float,2>& out)
{
    const auto it=values.find(key); if(it==values.end())return false;
    std::istringstream s(it->second); return static_cast<bool>(s>>out[0]>>out[1]);
}
RmlUiDesign::Values LoadDesignValues()
{
    Rml::FileInterface* files=Rml::GetFileInterface(); if(!files)return {};
    Rml::String c; if(!files->LoadFile(kDocumentPath,c)||c.empty())return {};
    Rml::StreamMemory s(reinterpret_cast<const Rml::byte*>(c.data()),c.size());
    return RmlUiDesign::Parse(&s);
}
void SetText(Rml::Element* e,const std::string& t)
{
    if(!e)return; e->SetInnerRML("");
    if(Rml::ElementDocument* d=e->GetOwnerDocument())e->AppendChild(d->CreateTextNode(t));
}
}

class RmlDuelWatchPanel::Impl
{
public:
    Impl():host_(kDocumentPath){}
    ~Impl(){Release();}

    bool Load(bool show)
    {
        if(host_.IsLoaded())return show?Show():true;
        if(!host_.Load(show))return false;
        document_=host_.GetDocument(); if(!document_)return false;
        const auto values=LoadDesignValues();
        (void)ParsePair(values,"Panel-Size",panelSize_);
        panel_=document_->GetElementById("panel");
        title_=document_->GetElementById("tfTitle");
        explain_=document_->GetElementById("tfExplain");
        Rml::Element* drag=document_->GetElementById("btnDrag");
        Rml::Element* close=document_->GetElementById("btnClose");
        if(!panel_||!title_||!explain_||!drag||!close){Release();return false;}
        close_.Bind(close);
        mover_.Bind(panel_,drag);

        for(int i=0;i<4;++i)
        {
            const Rml::String suffix=std::to_string(i);
            titles_[i]=document_->GetElementById(Rml::String("tfDuelTitle")+suffix);
            progress_[i]=document_->GetElementById(Rml::String("tfProgressDuel")+suffix);
            Rml::Element* button=document_->GetElementById(Rml::String("btnDuelWatch")+suffix);
            labels_[i]=document_->GetElementById(Rml::String("btnDuelWatch")+suffix+"-label");
            if(!titles_[i]||!progress_[i]||!button||!labels_[i]){Release();return false;}
            watch_[i].Bind(button);
        }

        hostVisible_=show;
        return ApplyState(state_);
    }

    bool Show()
    {
        if(!host_.IsLoaded()&&!Load(false))return false;
        hostVisible_=true;
        const bool ok=host_.Show(); if(ok)(void)ApplyState(state_); return ok;
    }
    bool Hide()
    {
        hostVisible_=false; if(panel_)panel_->SetProperty("display","none"); return host_.Hide();
    }
    void Release()
    {
        close_.Unbind(); mover_.Unbind(); for(auto& b:watch_)b.Unbind();
        titles_.fill(nullptr);progress_.fill(nullptr);labels_.fill(nullptr);
        panel_=title_=explain_=nullptr;document_=nullptr;join_.reset();closeRequest_=false;host_.Release();
    }
    bool IsLoaded() const noexcept{return host_.IsLoaded()&&document_!=nullptr;}

    bool ApplyState(const State& state)
    {
        state_=state; if(!IsLoaded())return false;
        const bool visible=hostVisible_&&state_.visible&&state_.viewportWidth>0&&state_.viewportHeight>0;
        panel_->SetProperty("display",visible?"block":"none"); if(!visible)return true;
        panel_->SetProperty("left",PixelValue(state_.x));panel_->SetProperty("top",PixelValue(state_.y));
        panel_->SetProperty("width",PixelValue(panelSize_[0]));panel_->SetProperty("height",PixelValue(panelSize_[1]));
        SetText(title_,state_.title);SetText(explain_,state_.explain);

        mover_.SetMetrics(
            static_cast<float>(state_.viewportWidth),static_cast<float>(state_.viewportHeight),
            panelSize_[0],panelSize_[1],0.0f,0.0f);
        mover_.SetPosition(state_.x,state_.y);

        for(int i=0;i<4;++i)
        {
            const auto& c=state_.channels[i];
            SetText(titles_[i],std::string("Arena ")+std::to_string(i+1));
            SetText(progress_[i],c.enabled?(c.user1+"  VS  "+c.user2):"No duel");
            SetText(labels_[i],"Watch");
            watch_[i].SetEnabled(c.enabled&&c.joinable);
            (void)watch_[i].Update();
        }
        document_->UpdateDocument(); return true;
    }

    bool Update()
    {
        if(!IsLoaded())return false;
        bool changed=close_.Update()|mover_.ConsumePositionChanged();
        if(close_.ConsumeClicked()){closeRequest_=true;changed=true;}
        for(int i=0;i<4;++i)
        {
            changed|=watch_[i].Update();
            if(watch_[i].ConsumeClicked()){join_=i;changed=true;}
        }
        return changed;
    }

    std::optional<int> ConsumeJoinChannel(){return std::exchange(join_,std::nullopt);}
    bool ConsumeCloseRequest() noexcept{return std::exchange(closeRequest_,false);}

private:
    RmlDocumentHost host_;
    Rml::ElementDocument* document_=nullptr;
    Rml::Element* panel_=nullptr;
    Rml::Element* title_=nullptr;
    Rml::Element* explain_=nullptr;
    std::array<Rml::Element*,4> titles_{},progress_{},labels_{};
    std::array<RmlMuButton,4> watch_{};
    RmlMuButton close_;
    RmlMuMovablePanel mover_;
    std::array<float,2> panelSize_{327.0f,639.0f};
    bool hostVisible_=false;
    State state_;
    std::optional<int> join_;
    bool closeRequest_=false;
};

RmlDuelWatchPanel::RmlDuelWatchPanel():m_impl(std::make_unique<Impl>()){}
RmlDuelWatchPanel::~RmlDuelWatchPanel()=default;
RmlDuelWatchPanel::RmlDuelWatchPanel(RmlDuelWatchPanel&&) noexcept=default;
RmlDuelWatchPanel& RmlDuelWatchPanel::operator=(RmlDuelWatchPanel&&) noexcept=default;
bool RmlDuelWatchPanel::Load(bool show){return m_impl&&m_impl->Load(show);}
bool RmlDuelWatchPanel::Show(){return m_impl&&m_impl->Show();}
bool RmlDuelWatchPanel::Hide(){return m_impl&&m_impl->Hide();}
void RmlDuelWatchPanel::Release(){if(m_impl)m_impl->Release();}
bool RmlDuelWatchPanel::IsLoaded() const noexcept{return m_impl&&m_impl->IsLoaded();}
bool RmlDuelWatchPanel::ApplyState(const State& state){return m_impl&&m_impl->ApplyState(state);}
bool RmlDuelWatchPanel::Update(){return m_impl&&m_impl->Update();}
std::optional<int> RmlDuelWatchPanel::ConsumeJoinChannel(){return m_impl?m_impl->ConsumeJoinChannel():std::nullopt;}
bool RmlDuelWatchPanel::ConsumeCloseRequest() noexcept{return m_impl&&m_impl->ConsumeCloseRequest();}
}

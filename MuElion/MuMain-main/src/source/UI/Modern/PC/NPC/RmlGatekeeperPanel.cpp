#include "stdafx.h"
#include "UI/Modern/PC/NPC/RmlGatekeeperPanel.h"

#include "UI/Modern/RmlDocumentHost.h"
#include "UI/Modern/RmlMuButton.h"
#include "UI/Modern/RmlMuMovablePanel.h"
#include "UI/Modern/RmlMuOptionStepper.h"
#include "UI/Modern/RmlUiDesign.h"

#include <RmlUi/Core/Core.h>
#include <RmlUi/Core/Element.h>
#include <RmlUi/Core/ElementDocument.h>
#include <RmlUi/Core/FileInterface.h>
#include <RmlUi/Core/StreamMemory.h>

#include <algorithm>
#include <array>
#include <sstream>
#include <utility>

namespace UI::Modern::PC::NPC
{
namespace
{
constexpr const char* kDocumentPath="Data/UI/PC/NPCs/gatekeeper.rml";
Rml::Element* ById(Rml::ElementDocument*d,const char*id){return d?d->GetElementById(id):nullptr;}
void Visible(Rml::Element*e,bool v){if(e)e->SetProperty("display",v?"block":"none");}
void Text(Rml::Element*e,const std::string&s){if(!e)return;e->SetInnerRML("");if(auto*d=e->GetOwnerDocument())e->AppendChild(d->CreateTextNode(s));}
bool ParsePair(const RmlUiDesign::Values&v,const char*k,std::array<float,2>&o){auto i=v.find(k);if(i==v.end())return false;std::istringstream s(i->second);return static_cast<bool>(s>>o[0]>>o[1]);}
RmlUiDesign::Values Design(){auto*f=Rml::GetFileInterface();if(!f)return{};Rml::String t;if(!f->LoadFile(kDocumentPath,t)||t.empty())return{};Rml::StreamMemory s(reinterpret_cast<const Rml::byte*>(t.data()),t.size());return RmlUiDesign::Parse(&s);}
} // namespace

class RmlGatekeeperPanel::Impl
{
public:
 Impl():host_(kDocumentPath){}
 ~Impl(){Release();}

 bool Load(bool show)
 {
  if(host_.IsLoaded())return show?Show():true;
  if(!host_.Load(show))return false;
  document_=host_.GetDocument();if(!document_)return false;
  const auto d=Design();(void)ParsePair(d,"Panel-Size",panelSize_);(void)ParsePair(d,"Panel-Reference",reference_);(void)ParsePair(d,"Panel-Initial",initial_);
  if(!Bind()){Release();return false;}hostVisible_=show;return ApplyState(state_);
 }
 bool Show(){if(!host_.IsLoaded()&&!Load(false))return false;hostVisible_=true;const bool ok=host_.Show();if(ok)(void)ApplyState(state_);return ok;}
 bool Hide(){hostVisible_=false;Visible(panel_,false);return host_.Hide();}
 void Release(){close_.Unbind();enter_.Unbind();set_.Unbind();public_.Unbind();stepper_.Unbind();mover_.Unbind();document_=nullptr;panel_=drag_=title_=message_=limit_=fee_=setting_=feeHelp_=feeLabel_=publicLabel_=nullptr;pending_.reset();host_.Release();}
 bool IsLoaded()const noexcept{return host_.IsLoaded()&&document_;}

 bool ApplyState(const State& state)
 {
  state_=state;state_.addEntranceFee=std::max(1,state_.addEntranceFee);state_.maxEntranceFee=std::max(0,state_.maxEntranceFee);state_.viewEntranceFee=std::clamp(state_.viewEntranceFee,0,state_.maxEntranceFee);
  if(!IsLoaded())return false;
  const bool visible=hostVisible_&&state_.visible&&state_.viewportWidth>0&&state_.viewportHeight>0;Visible(panel_,visible);if(!visible)return true;
  Text(title_,state_.title);

  const bool master=state_.mode==Mode::GuildMaster;
  const bool member=state_.mode==Mode::GuildMember;
  if(master)
  {
   Text(message_,"Configure castle entrance access and fee.");
   Text(limit_,"Entrance restriction");
   Text(fee_,"Current fee: "+std::to_string(state_.entranceFee)+" Zen");
   Text(setting_,"Entrance fee setting");
   Text(feeHelp_,"Maximum "+std::to_string(state_.maxEntranceFee)+" Zen; step "+std::to_string(state_.addEntranceFee)+" Zen");
  }
  else if(member)
  {
   Text(message_,"Would you like to enter?");
   Text(limit_,"Guild member access");
   Text(fee_,"");
   Text(setting_,"");Text(feeHelp_,"");
  }
  else if(state_.publicEntry)
  {
   Text(message_,"Pay the entrance fee to enter.");
   Text(limit_,"Public entrance");
   Text(fee_,"Entrance fee: "+std::to_string(state_.entranceFee)+" Zen");
   Text(setting_,"");Text(feeHelp_,"");
  }
  else
  {
   Text(message_,"Entering is not allowed. Approval from the castle lord is required.");
   Text(limit_,"Private entrance");
   Text(fee_,"");Text(setting_,"");Text(feeHelp_,"");
  }

  Text(feeLabel_,std::to_string(state_.viewEntranceFee)+" Zen");
  Text(publicLabel_,state_.publicEntry?"Open to non-members":"Guild members only");
  Visible(ById(document_,"opZen"),master);
  Visible(ById(document_,"btnSetConfirm"),master);
  Visible(ById(document_,"cbEnterCheck"),master);

  const bool canEnter=member||master||(state_.publicEntry&&state_.playerGold>=state_.entranceFee);
  enter_.SetEnabled(canEnter);(void)enter_.Update();
  set_.SetEnabled(master);public_.SetEnabled(master);(void)set_.Update();(void)public_.Update();
  if(auto*e=ById(document_,"cbEnterCheck"))e->SetClass("checked",state_.publicEntry);

  const int maxIndex=state_.maxEntranceFee/state_.addEntranceFee;
  const int current=state_.viewEntranceFee/state_.addEntranceFee;
  (void)stepper_.SetState(current,maxIndex,master);

  mover_.SetMetrics((float)state_.viewportWidth,(float)state_.viewportHeight,panelSize_[0],panelSize_[1],0,0);mover_.SetPosition(state_.x,state_.y);
  document_->UpdateDocument();return true;
 }

 bool Update()
 {
  if(!IsLoaded())return false;
  bool ch=close_.Update()|enter_.Update()|set_.Update()|public_.Update();
  if(close_.ConsumeClicked()){pending_=Action{ActionType::Close,false,0};ch=true;}
  if(enter_.ConsumeClicked()){pending_=Action{ActionType::Enter,false,0};ch=true;}
  if(set_.ConsumeClicked()){pending_=Action{ActionType::SetFee,false,state_.viewEntranceFee};ch=true;}
  if(public_.ConsumeClicked()){pending_=Action{ActionType::TogglePublic,!state_.publicEntry,0};ch=true;}
  if(auto value=stepper_.ConsumeChangedValue())
  {
   state_.viewEntranceFee=std::clamp(*value*state_.addEntranceFee,0,state_.maxEntranceFee);
   Text(feeLabel_,std::to_string(state_.viewEntranceFee)+" Zen");
   pending_=Action{ActionType::FeeChanged,false,state_.viewEntranceFee};ch=true;
  }
  ch|=mover_.ConsumePositionChanged();return ch;
 }
 std::optional<Action> ConsumeAction(){return std::exchange(pending_,std::nullopt);}

private:
 bool Bind()
 {
  panel_=ById(document_,"panel");drag_=ById(document_,"btnDrag");title_=ById(document_,"tfTitle");message_=ById(document_,"taEnterMent");limit_=ById(document_,"tfEnterLimit");fee_=ById(document_,"tfEnterFee");setting_=ById(document_,"tfFeeSetting");feeHelp_=ById(document_,"taEnterFeeMent");feeLabel_=ById(document_,"opZen-label");publicLabel_=ById(document_,"cbEnterCheck-label");
  auto*c=ById(document_,"btnClose");auto*s=ById(document_,"btnSetConfirm");auto*e=ById(document_,"btnEnter");auto*p=ById(document_,"cbEnterCheck");auto*root=ById(document_,"opZen");auto*prev=ById(document_,"prevBtn");auto*next=ById(document_,"nextBtn");
  if(!panel_||!drag_||!title_||!message_||!limit_||!fee_||!setting_||!feeHelp_||!feeLabel_||!publicLabel_||!c||!s||!e||!p||!root||!prev||!next)return false;
  close_.Bind(c);set_.Bind(s);enter_.Bind(e);public_.Bind(p);stepper_.Bind(root,prev,next);mover_.Bind(panel_,drag_);return true;
 }
 RmlDocumentHost host_;Rml::ElementDocument*document_=nullptr;bool hostVisible_=false;
 Rml::Element*panel_=nullptr,*drag_=nullptr,*title_=nullptr,*message_=nullptr,*limit_=nullptr,*fee_=nullptr,*setting_=nullptr,*feeHelp_=nullptr,*feeLabel_=nullptr,*publicLabel_=nullptr;
 RmlMuButton close_,enter_,set_,public_;RmlMuOptionStepper stepper_;RmlMuMovablePanel mover_;
 std::array<float,2>panelSize_{327,639},reference_{640,480},initial_{20,20};State state_;std::optional<Action>pending_;
};

RmlGatekeeperPanel::RmlGatekeeperPanel():m_impl(std::make_unique<Impl>()){}
RmlGatekeeperPanel::~RmlGatekeeperPanel()=default;
RmlGatekeeperPanel::RmlGatekeeperPanel(RmlGatekeeperPanel&&) noexcept=default;
RmlGatekeeperPanel& RmlGatekeeperPanel::operator=(RmlGatekeeperPanel&&) noexcept=default;
bool RmlGatekeeperPanel::Load(bool s){return m_impl&&m_impl->Load(s);}
bool RmlGatekeeperPanel::Show(){return m_impl&&m_impl->Show();}
bool RmlGatekeeperPanel::Hide(){return m_impl&&m_impl->Hide();}
void RmlGatekeeperPanel::Release(){if(m_impl)m_impl->Release();}
bool RmlGatekeeperPanel::IsLoaded()const noexcept{return m_impl&&m_impl->IsLoaded();}
bool RmlGatekeeperPanel::ApplyState(const State&s){return m_impl&&m_impl->ApplyState(s);}
bool RmlGatekeeperPanel::Update(){return m_impl&&m_impl->Update();}
std::optional<RmlGatekeeperPanel::Action> RmlGatekeeperPanel::ConsumeAction(){return m_impl?m_impl->ConsumeAction():std::nullopt;}

} // namespace UI::Modern::PC::NPC

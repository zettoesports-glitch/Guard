#include "stdafx.h"
#include "UI/Modern/PC/Guild/RmlGuildInfoPanel.h"

#include "UI/Modern/RmlDocumentHost.h"
#include "UI/Modern/RmlMuButton.h"
#include "UI/Modern/RmlMuMovablePanel.h"
#include "UI/Modern/RmlMuScrollBar.h"
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

namespace UI::Modern::PC::Guild
{
namespace
{
constexpr const char* kDocumentPath="Data/UI/PC/Guild/guild_info.rml";
constexpr std::size_t kMemberRows=12;
constexpr std::size_t kUnionRows=6;
constexpr std::size_t kRivalRows=8;
constexpr std::size_t kNoticeRows=5;

Rml::Element* ById(Rml::ElementDocument*d,const char*id)
{return d?d->GetElementById(id):nullptr;}
void Text(Rml::Element*e,const std::string&s)
{
 if(!e)return;e->SetInnerRML("");
 if(auto*d=e->GetOwnerDocument())e->AppendChild(d->CreateTextNode(s));
}
void Visible(Rml::Element*e,bool v)
{if(e)e->SetProperty("display",v?"block":"none");}
bool ParsePair(const RmlUiDesign::Values&v,const char*k,std::array<float,2>&o)
{
 auto i=v.find(k);if(i==v.end())return false;
 std::istringstream s(i->second);return static_cast<bool>(s>>o[0]>>o[1]);
}
RmlUiDesign::Values Design()
{
 auto*f=Rml::GetFileInterface();if(!f)return{};
 Rml::String t;if(!f->LoadFile(kDocumentPath,t)||t.empty())return{};
 Rml::StreamMemory s(reinterpret_cast<const Rml::byte*>(t.data()),t.size());
 return RmlUiDesign::Parse(&s);
}
std::vector<std::string> SplitLines(const std::string& text)
{
 std::vector<std::string> rows;std::istringstream s(text);std::string line;
 while(std::getline(s,line))rows.push_back(line);
 if(rows.empty()&&!text.empty())rows.push_back(text);
 return rows;
}
} // namespace

class RmlGuildInfoPanel::Impl
{
public:
 Impl():host_(kDocumentPath){}
 ~Impl(){Release();}

 bool Load(bool show)
 {
  if(host_.IsLoaded())return show?Show():true;
  if(!host_.Load(show))return false;
  document_=host_.GetDocument();if(!document_)return false;
  const auto d=Design();
  (void)ParsePair(d,"Panel-Size",panelSize_);
  (void)ParsePair(d,"Panel-Reference",reference_);
  (void)ParsePair(d,"Panel-Initial",initial_);
  if(!Bind()){Release();return false;}
  hostVisible_=show;return ApplyState(state_);
 }
 bool Show(){if(!host_.IsLoaded()&&!Load(false))return false;hostVisible_=true;auto ok=host_.Show();if(ok)(void)ApplyState(state_);return ok;}
 bool Hide(){hostVisible_=false;Visible(panel_,false);return host_.Hide();}
 void Release()
 {
  close_.Unbind();for(auto&b:tabs_)b.Unbind();
  position_.Unbind();clearPosition_.Unbind();fire_.Unbind();out_.Unbind();
  unionDisbandGuild_.Unbind();unionDisbandUnion_.Unbind();
  rivalAdd_.Unbind();rivalDelete_.Unbind();
  for(auto&s:scrolls_)s.Unbind();mover_.Unbind();
  memberButtons_.clear();unionButtons_.clear();rivalButtons_.clear();
  document_=nullptr;panel_=drag_=title_=infoText_=nullptr;
  groups_.fill(nullptr);lists_.fill(nullptr);pending_.reset();host_.Release();
 }
 bool IsLoaded()const noexcept{return host_.IsLoaded()&&document_;}

 bool ApplyState(const State& state)
 {
  state_=state;
  if(!IsLoaded())return false;
  const bool vis=hostVisible_&&state_.visible&&state_.viewportWidth>0&&state_.viewportHeight>0;
  Visible(panel_,vis);if(!vis)return true;
  Text(title_,state_.title);
  Text(infoText_,state_.hasGuild?"":"Join or create a guild to use this window.");
  mover_.SetMetrics((float)state_.viewportWidth,(float)state_.viewportHeight,
      panelSize_[0],panelSize_[1],0,0);
  mover_.SetPosition(state_.x,state_.y);

  for(std::size_t i=0;i<4;++i)
  {
   const bool selected=static_cast<std::size_t>(state_.tab)==i;
   Visible(groups_[i],state_.hasGuild&&selected);
   if(auto*e=tabElements_[i])e->SetClass("selected",selected);
  }

  Text(ById(document_,"tfGuildName"),state_.guildName);
  Text(ById(document_,"tfGuildPointValue"),std::to_string(state_.guildScore));
  Text(ById(document_,"tfGuildMemberNumValue"),
       state_.memberCapacity > 0
           ? std::to_string(state_.memberCount)+" / "+std::to_string(state_.memberCapacity)
           : std::to_string(state_.memberCount));
  Text(ById(document_,"tfRivalGuildValue"),state_.rivalGuild.empty()?"-":state_.rivalGuild);
  Text(ById(document_,"taGuildNotice"),state_.notice);
  Text(ById(document_,"btnDisband-label"),state_.isGuildMaster?"Disband":"Leave");

  ApplyMark();
  RebuildMembers();
  RebuildUnions();
  RebuildRivals();
  RebuildNotice();

  position_.SetEnabled(state_.isGuildMaster&&selectedMember_.has_value());
  clearPosition_.SetEnabled(state_.isGuildMaster&&selectedMember_.has_value());
  fire_.SetEnabled(state_.isGuildMaster&&selectedMember_.has_value());
  unionDisbandGuild_.SetEnabled(state_.isGuildMaster&&selectedUnion_.has_value());
  unionDisbandUnion_.SetEnabled(state_.isGuildMaster);
  rivalDelete_.SetEnabled(state_.isGuildMaster&&selectedRival_.has_value());
  rivalAdd_.SetEnabled(state_.isGuildMaster);
  (void)position_.Update();(void)clearPosition_.Update();(void)fire_.Update();
  (void)unionDisbandGuild_.Update();(void)unionDisbandUnion_.Update();
  (void)rivalAdd_.Update();(void)rivalDelete_.Update();
  document_->UpdateDocument();return true;
 }

 bool Update()
 {
  if(!IsLoaded())return false;
  bool ch=close_.Update();
  if(close_.ConsumeClicked()){pending_=Action{ActionType::Close,0,state_.tab};ch=true;}
  for(std::size_t i=0;i<tabs_.size();++i)
  {
   ch|=tabs_[i].Update();
   if(tabs_[i].ConsumeClicked())
   {
    state_.tab=static_cast<Tab>(i);
    pending_=Action{ActionType::SelectTab,i,state_.tab};
    (void)ApplyState(state_);ch=true;
   }
  }
  ch|=PollAction(position_,ActionType::Position);
  ch|=PollAction(clearPosition_,ActionType::ClearPosition);
  ch|=PollAction(fire_,ActionType::FireMember);
  ch|=PollAction(out_,ActionType::DisbandOrLeave);
  ch|=PollAction(unionDisbandGuild_,ActionType::UnionDisbandGuild);
  ch|=PollAction(unionDisbandUnion_,ActionType::UnionDisbandUnion);
  ch|=PollAction(rivalAdd_,ActionType::RivalAdd);
  ch|=PollAction(rivalDelete_,ActionType::RivalDelete);

  ch|=PollRows(memberButtons_,selectedMember_,ActionType::SelectMember);
  ch|=PollRows(unionButtons_,selectedUnion_,ActionType::SelectUnion);
  ch|=PollRows(rivalButtons_,selectedRival_,ActionType::SelectRival);

  const std::array<std::size_t,4> sizes{
      SplitLines(state_.notice).size(),state_.members.size(),
      state_.unions.size(),state_.rivals.size()};
  const std::array<std::size_t,4> visible{kNoticeRows,kMemberRows,kUnionRows,kRivalRows};
  for(std::size_t i=0;i<scrolls_.size();++i)
  {
   const auto max=sizes[i]>visible[i]?sizes[i]-visible[i]:0u;
   (void)scrolls_[i].SetState(scrollStart_[i],max,visible[i],visible[i],max>0,true);
   if(auto p=scrolls_[i].ConsumeRequestedPosition())
   {
    scrollStart_[i]=std::min(*p,max);
    RebuildByIndex(i);ch=true;
   }
  }
  ch|=mover_.ConsumePositionChanged();return ch;
 }

 std::optional<Action> ConsumeAction(){return std::exchange(pending_,std::nullopt);}

private:
 bool Bind()
 {
  panel_=ById(document_,"panel");drag_=ById(document_,"btnDrag");
  title_=ById(document_,"tfTitle");infoText_=ById(document_,"taGuildInfo");
  if(!panel_||!drag_||!title_||!infoText_)return false;
  auto*close=ById(document_,"btnClose");if(!close)return false;close_.Bind(close);mover_.Bind(panel_,drag_);
  const char*tabIds[]={"btnToggleInfo","btnToggleMember","btnToggleUnion","btnToggleRival"};
  const char*groupIds[]={"mcInfo","mcMember","mcUnion","mcRival"};
  for(std::size_t i=0;i<4;++i){tabElements_[i]=ById(document_,tabIds[i]);groups_[i]=ById(document_,groupIds[i]);if(!tabElements_[i]||!groups_[i])return false;tabs_[i].Bind(tabElements_[i]);}
  lists_[0]=ById(document_,"taGuildNotice");lists_[1]=ById(document_,"slMember");
  lists_[2]=ById(document_,"slUnion");lists_[3]=ById(document_,"slRival");
  if(!lists_[0]||!lists_[1]||!lists_[2]||!lists_[3])return false;
  const char*scrollIds[]={"sbGuildNotice","sbMember","sbUnion","sbRival"};
  for(std::size_t i=0;i<4;++i)if(!scrolls_[i].Bind(ById(document_,scrollIds[i])))return false;
  BindButton(position_,"btnPosition");BindButton(clearPosition_,"btnClear");BindButton(fire_,"btnFire");
  BindButton(out_,"btnDisband");BindButton(unionDisbandGuild_,"btnUnionDisbandGuild");
  BindButton(unionDisbandUnion_,"btnUnionDisbandUnion");BindButton(rivalAdd_,"btnRivalAdd");BindButton(rivalDelete_,"btnRivalDelete");
  return true;
 }
 void BindButton(RmlMuButton&b,const char*id){if(auto*e=ById(document_,id))b.Bind(e);}

 bool PollAction(RmlMuButton&b,ActionType t)
 {
  bool ch=b.Update();if(b.ConsumeClicked())
  {
   std::size_t index=0;
   if(t==ActionType::Position||t==ActionType::ClearPosition||t==ActionType::FireMember)index=selectedMember_.value_or(0);
   else if(t==ActionType::UnionDisbandGuild)index=selectedUnion_.value_or(0);
   else if(t==ActionType::RivalDelete)index=selectedRival_.value_or(0);
   pending_=Action{t,index,state_.tab};ch=true;
  }return ch;
 }
 bool PollRows(std::vector<std::unique_ptr<RmlMuButton>>&buttons,std::optional<std::size_t>&selection,ActionType type)
 {
  bool ch=false;for(std::size_t i=0;i<buttons.size();++i){ch|=buttons[i]->Update();if(buttons[i]->ConsumeClicked()){selection=scrollStart_[ListIndex(type)]+i;pending_=Action{type,*selection,state_.tab};ch=true;(void)ApplyState(state_);}}return ch;
 }
 std::size_t ListIndex(ActionType type)const
 {
  if(type==ActionType::SelectMember)return 1;
  if(type==ActionType::SelectUnion)return 2;
  return 3;
 }

 void ApplyMark()
 {
  auto*grid=ById(document_,"mcGuildMark");if(!grid)return;
  grid->SetInnerRML("");
  for(std::size_t i=0;i<MarkCellCount;++i)
  {
   auto cell=document_->CreateElement("div");cell->SetClass("guild-mark-cell",true);
   cell->SetClass(Rml::String("palette-color-")+std::to_string(state_.guildMark[i]&0x0f),true);
   grid->AppendChild(std::move(cell));
  }
 }
 void RebuildByIndex(std::size_t i){if(i==0)RebuildNotice();else if(i==1)RebuildMembers();else if(i==2)RebuildUnions();else RebuildRivals();}
 void RebuildNotice()
 {
  const auto rows=SplitLines(state_.notice);lists_[0]->SetInnerRML("");
  const auto end=std::min(rows.size(),scrollStart_[0]+kNoticeRows);
  for(std::size_t i=scrollStart_[0];i<end;++i){auto row=document_->CreateElement("div");row->SetClass("guild-row",true);row->AppendChild(document_->CreateTextNode(rows[i]));lists_[0]->AppendChild(std::move(row));}
 }
 void RebuildMembers()
 {
  memberButtons_.clear();lists_[1]->SetInnerRML("");
  const auto end=std::min(state_.members.size(),scrollStart_[1]+kMemberRows);
  for(std::size_t i=scrollStart_[1];i<end;++i)
  {
   const auto&m=state_.members[i];auto row=document_->CreateElement("div");row->SetClass("guild-row",true);row->SetClass("selected",selectedMember_&&*selectedMember_==i);
   auto a=document_->CreateElement("span");a->SetClass("guild-col-name",true);a->AppendChild(document_->CreateTextNode(m.name));
   auto b=document_->CreateElement("span");b->SetClass("guild-col-position",true);b->AppendChild(document_->CreateTextNode(m.position));
   auto c=document_->CreateElement("span");c->SetClass("guild-col-server",true);c->AppendChild(document_->CreateTextNode(m.server));
   row->AppendChild(std::move(a));row->AppendChild(std::move(b));row->AppendChild(std::move(c));
   auto*raw=lists_[1]->AppendChild(std::move(row));auto button=std::make_unique<RmlMuButton>();button->Bind(raw);memberButtons_.push_back(std::move(button));
  }
 }
 void RebuildUnions()
 {
  unionButtons_.clear();lists_[2]->SetInnerRML("");
  const auto end=std::min(state_.unions.size(),scrollStart_[2]+kUnionRows);
  for(std::size_t i=scrollStart_[2];i<end;++i)
  {
   const auto&g=state_.unions[i];auto row=document_->CreateElement("div");row->SetClass("guild-row",true);row->SetClass("selected",selectedUnion_&&*selectedUnion_==i);
   row->AppendChild(document_->CreateTextNode(g.name+"  "+std::to_string(g.memberCount)));
   auto*raw=lists_[2]->AppendChild(std::move(row));auto button=std::make_unique<RmlMuButton>();button->Bind(raw);unionButtons_.push_back(std::move(button));
  }
 }
 void RebuildRivals()
 {
  rivalButtons_.clear();lists_[3]->SetInnerRML("");
  const auto end=std::min(state_.rivals.size(),scrollStart_[3]+kRivalRows);
  for(std::size_t i=scrollStart_[3];i<end;++i)
  {
   auto row=document_->CreateElement("div");row->SetClass("guild-row",true);row->SetClass("selected",selectedRival_&&*selectedRival_==i);
   row->AppendChild(document_->CreateTextNode(state_.rivals[i]));
   auto*raw=lists_[3]->AppendChild(std::move(row));auto button=std::make_unique<RmlMuButton>();button->Bind(raw);rivalButtons_.push_back(std::move(button));
  }
 }

 RmlDocumentHost host_;Rml::ElementDocument*document_=nullptr;bool hostVisible_=false;
 Rml::Element*panel_=nullptr,*drag_=nullptr,*title_=nullptr,*infoText_=nullptr;
 std::array<Rml::Element*,4>groups_{};std::array<Rml::Element*,4>lists_{};std::array<Rml::Element*,4>tabElements_{};
 RmlMuButton close_;std::array<RmlMuButton,4>tabs_{};
 RmlMuButton position_,clearPosition_,fire_,out_,unionDisbandGuild_,unionDisbandUnion_,rivalAdd_,rivalDelete_;
 std::array<RmlMuScrollBar,4>scrolls_{};RmlMuMovablePanel mover_;
 std::vector<std::unique_ptr<RmlMuButton>>memberButtons_,unionButtons_,rivalButtons_;
 std::array<std::size_t,4>scrollStart_{};
 std::optional<std::size_t>selectedMember_,selectedUnion_,selectedRival_;
 std::array<float,2>panelSize_{327,639},reference_{640,480},initial_{20,20};
 State state_;std::optional<Action>pending_;
};

RmlGuildInfoPanel::RmlGuildInfoPanel():m_impl(std::make_unique<Impl>()){}
RmlGuildInfoPanel::~RmlGuildInfoPanel()=default;
RmlGuildInfoPanel::RmlGuildInfoPanel(RmlGuildInfoPanel&&) noexcept=default;
RmlGuildInfoPanel& RmlGuildInfoPanel::operator=(RmlGuildInfoPanel&&) noexcept=default;
bool RmlGuildInfoPanel::Load(bool s){return m_impl&&m_impl->Load(s);}
bool RmlGuildInfoPanel::Show(){return m_impl&&m_impl->Show();}
bool RmlGuildInfoPanel::Hide(){return m_impl&&m_impl->Hide();}
void RmlGuildInfoPanel::Release(){if(m_impl)m_impl->Release();}
bool RmlGuildInfoPanel::IsLoaded()const noexcept{return m_impl&&m_impl->IsLoaded();}
bool RmlGuildInfoPanel::ApplyState(const State&s){return m_impl&&m_impl->ApplyState(s);}
bool RmlGuildInfoPanel::Update(){return m_impl&&m_impl->Update();}
std::optional<RmlGuildInfoPanel::Action> RmlGuildInfoPanel::ConsumeAction(){return m_impl?m_impl->ConsumeAction():std::nullopt;}

} // namespace UI::Modern::PC::Guild

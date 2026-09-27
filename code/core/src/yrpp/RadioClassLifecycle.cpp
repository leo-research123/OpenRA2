// Native radio ownership; original 0x0065A750.
#include "yrpp/RadioClass.h"
#include "yrpp/TechnoClass.h"
#include "yrpp/HouseClass.h"
namespace {AbstractClass* radio_scratch=nullptr;}
RadioClass::RadioClass() noexcept
 : MissionClass(),
 LastCommands{},
 RadioLinks(1) { RadioLinks[0]=nullptr; }
RadioClass::~RadioClass(){
 RadioLinks.~VectorClass();
}
bool RadioClass::Limbo() {
    if(!InLimbo)SendToEachLink(RadioCommand::NotifyUnlink);
    return ObjectClass::Limbo();
}
RadioCommand RadioClass::SendToFirstLink(RadioCommand command) {
    // Original shared in/out scratch at 0xA8EC30. The constructor provides
    // the original one null radio slot, so an unlinked GI returns zero.
    auto* recipient=GetNthLink();
    return recipient?SendCommandWithData(command,radio_scratch,recipient):RadioCommand::AnswerInvalid;
}
void RadioClass::SendToEachLink(RadioCommand command) {
    for(int i=0;i<RadioLinks.Capacity;++i)
        if(auto* recipient=RadioLinks[i])SendCommandWithData(command,radio_scratch,recipient);
}

RadioCommand RadioClass::SendCommand(RadioCommand command,TechnoClass* recipient){return SendCommandWithData(command,radio_scratch,recipient);}
RadioCommand RadioClass::SendCommandWithData(RadioCommand command,AbstractClass*& data,TechnoClass* recipient){
 if(!recipient)recipient=GetNthLink();if(!recipient)return RadioCommand::AnswerInvalid;
 auto* sender=(AbstractFlags&::AbstractFlags::Techno)!=::AbstractFlags::None?static_cast<TechnoClass*>(this):nullptr;
 if(command==RadioCommand::NotifyUnlink)for(int i=0;i<RadioLinks.Capacity;++i)if(RadioLinks[i]==recipient)RadioLinks[i]=nullptr;
 if(command!=RadioCommand::RequestLink)return recipient->ReceiveCommand(sender,command,data);
 int free=-1;
 for(int i=0;i<RadioLinks.Capacity;++i){if(free==-1&&!RadioLinks[i])free=i;if(RadioLinks[i]==recipient)return RadioCommand::AnswerPositive;}
 if(free==-1){SendCommand(RadioCommand::NotifyUnlink,GetNthLink());free=0;}
 if(recipient->ReceiveCommand(sender,command,data)!=RadioCommand::AnswerPositive)return RadioCommand::AnswerNegative;
 RadioLinks[free]=recipient;return RadioCommand::AnswerPositive;
}
RadioCommand RadioClass::ReceiveCommand(TechnoClass* sender,RadioCommand command,AbstractClass*& data){
 if(command!=LastCommands[0]){LastCommands[2]=LastCommands[1];LastCommands[1]=LastCommands[0];LastCommands[0]=command;}
 if(command==RadioCommand::NotifyUnlink){const int index=FindLinkIndex(sender);if(index>=0){ObjectClass::ReceiveCommand(sender,command,data);RadioLinks[index]=nullptr;return RadioCommand::AnswerPositive;}}
 if(command!=RadioCommand::RequestLink||!Health)return ObjectClass::ReceiveCommand(sender,command,data);
 if(!sender->Owner->IsAlliedWith(this)||((AbstractFlags&::AbstractFlags::Techno)!=::AbstractFlags::None&&!static_cast<TechnoClass*>(this)->Owner->IsAlliedWith(sender)))return RadioCommand::AnswerNegative;
 if(ContainsLink(sender))return RadioCommand::AnswerPositive;
 for(int i=0;i<RadioLinks.Capacity;++i)if(!RadioLinks[i]){RadioLinks[i]=sender;return RadioCommand::AnswerPositive;}
 return RadioCommand::AnswerNegative;
}
int RadioClass::FindLinkIndex(const TechnoClass* link) const {if(link)for(int i=0;i<RadioLinks.Capacity;++i)if(RadioLinks[i]==link)return i;return -1;}
bool RadioClass::ContainsLink(const TechnoClass* link) const {return FindLinkIndex(link)>=0;}
bool RadioClass::HasFreeLink() const {return HasFreeLink(nullptr);}
bool RadioClass::HasFreeLink(const TechnoClass* ignore) const {for(int i=0;i<RadioLinks.Capacity;++i)if(!RadioLinks[i]||RadioLinks[i]==ignore)return true;return false;}
void RadioClass::SetLinkCount(int count){const int old=RadioLinks.Capacity;if(count>old){RadioLinks.SetCapacity(count);for(int i=old;i<count;++i)RadioLinks[i]=nullptr;}}

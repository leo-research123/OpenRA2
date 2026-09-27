// OpenTS ObjectClass::Receive_Message; YR 0x5F5320.
#include "yrpp/ObjectClass.h"
#include "yrpp/RulesClass.h"
RadioCommand ObjectClass::ReceiveCommand(TechnoClass*,RadioCommand command,AbstractClass*&){
 if(command==RadioCommand::RequestRedraw){Mark(MarkType::Change);return RadioCommand::AnswerPositive;}
 if(command==RadioCommand::QueryNeedRepair)return GetHealthPercentage()<RulesClass::Instance->ConditionGreen?RadioCommand::AnswerPositive:RadioCommand::AnswerNegative;
 return RadioCommand::AnswerInvalid;
}

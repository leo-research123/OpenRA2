#pragma once
// Internal parsing helpers; never expose STL at the original-class boundary.
#include "scenario_object_ini.hpp"
#include "type_registry.hpp"
#include <limits>
#include <bit>
#include <cstdint>
class TActionClass;
class TEventClass;
namespace game {
// Project-only adapters: cursor/status contract differs from original LoadFromINI.
// No exceptions escape; a failed record is discarded by its caller.
bool read_trigger_action_fields(TActionClass& action,const char*& cursor) noexcept;
bool read_trigger_event_fields(TEventClass& event,const char*& cursor) noexcept;

// Original ahtoi (0x412610): consume hex digits until the first non-hex,
// preserving 32-bit wrap and signed ID bits used by SwizzleManagerClass.
inline LONG scenario_hex_identity(const char* text) noexcept {
 std::uint32_t value=0;
 for(;text&&*text;++text){
  const unsigned char c=*text;
  const int digit=c>='0'&&c<='9'?c-'0':c>='A'&&c<='F'?c-'A'+10:c>='a'&&c<='f'?c-'a'+10:-1;
  if(digit<0)break;
  value=value*16u+unsigned(digit);
 }
 return std::bit_cast<LONG>(value);
}

// Original waypoint-name conversion 0x763690: A=0, Z=25, AA=26.
inline bool scenario_waypoint(std::string_view text,int& result) {
 if(text.empty()){result=-1;return true;}
 unsigned value=0;
 for(unsigned char c:text){
  if(c>='a'&&c<='z')c-= 'a'-'A';
  if(c<'A'||c>'Z'||value>(unsigned(std::numeric_limits<int>::max())-26)/26)return false;
  value=value*26+unsigned(c-'A'+1);
 }
 result=int(value)-1;return true;
}
inline std::string scenario_text(CCINIClass& ini,const char* section,const char* key){
 char buffer[4096]{};ini.ReadString(section,key,"",buffer,sizeof(buffer));return buffer;
}
inline bool script_token(const char*& cursor,std::string& value){
 if(!cursor||!*cursor)return false;
 const char* end=cursor;while(*end&&*end!=',')++end;
 value=std::string(scenario_trim(std::string_view(cursor,std::size_t(end-cursor))));
 cursor=*end?end+1:end;return true;
}
inline bool script_integer(const char*& cursor,int& value){
 std::string token;return script_token(cursor,token)&&scenario_number(token,value);
}
template<class T>T* scenario_reference(const std::string& id){
 if(id=="-1"||INIClass::IsBlankValue(id.c_str()))return nullptr;
 if(id.size()>=3)return T::FindOrAllocate(id.c_str());
 int index=-1;return scenario_number(id,index)&&index>=0&&index<T::Array.Count?T::Array[index]:nullptr;
}
}

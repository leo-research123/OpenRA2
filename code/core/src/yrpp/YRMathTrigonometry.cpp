// Original 0x004CACB0/0x004CAD00. Reconstruct the original quantized sine
// table mathematically; no binary asset is embedded in the native core.
#include "yrpp/YRMath.h"
#include <array>
#include <bit>
#include <cmath>
#include <cstdint>
#include <cfenv>
namespace {
float chop(double value){
 float result=float(value);
 if(std::abs(double(result))>std::abs(value))result=std::bit_cast<float>(std::bit_cast<std::uint32_t>(result)-1);
 return result;
}
const std::array<float,10240>& sine_table(){
 static const auto table=[](){std::array<float,10240> result{};
  // Rule integer parsing can leave FE_TOWARDZERO active. Host libm then
  // produces sin(pi/2) just below 1, unlike the target x87 table initializer.
  // Build deterministically, apply the target float chop explicitly, and
  // restore the caller's rounding contract before publishing the table.
  const int previous=std::fegetround();
  struct Restore {int mode;~Restore(){if(mode!=-1)std::fesetround(mode);}}restore{previous};
  std::fesetround(FE_TONEAREST);
  for(unsigned i=0;i<result.size();++i)result[i]=chop(std::sin(i*(Math::TwoPi/8192.0)));
  // x87 FSIN's range reduction leaves these residuals at pi and 2*pi.
  result[4096]=1.2246063538223773e-16f;result[8192]=-2.4492127076447545e-16f;
  return result;
 }();return table;
}
int scaled(double value){const double v=value*2607.594482421875;return std::isfinite(v)&&v>=-2147483648.0&&v<2147483648.0?int(v):0;}
}
double YRPP_CDECL Math::sin(double value){const int raw=scaled(value);int index=(raw/2)%8192;if(index<0)index+=8192;if((raw&1)&&index<8191)++index;return sine_table()[index];}
double YRPP_CDECL Math::cos(double value){const int raw=scaled(value);const int rem=(raw/2)%8192;int index=rem+(rem<0?10240:2048);if((raw&1)&&index<10239)++index;return sine_table()[index];}

double YRPP_CDECL Math::asin(double value){
 const int index=std::isfinite(value)?int((value+1.0)*2048.0):0;
 if(index<0||index>4096)return 0;
 return chop(std::asin(-1.0+index/2048.0));
}

#pragma once
class BitFont;
namespace game {
struct GameUiFrame;
// Internal destination adapter for the original BitFont string algorithm.
// No ownership, additional text state or public YRpp/host ABI.
int submit_bitfont_string(BitFont&,const wchar_t*,int,int,int,int,GameUiFrame&);
}

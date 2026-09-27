// Intentionally header/SDK-free; real project ABI checks live in ABI.h.
#if !defined(_WIN32) || !defined(_MSC_VER) || !defined(_M_IX86) || defined(__MINGW32__)
#error This smoke target requires Windows x86 Microsoft ABI.
#endif
static_assert(sizeof(void*) == 4 && sizeof(long) == 4 && sizeof(wchar_t) == 2);
struct Interface {
    virtual ~Interface() = default;
    virtual int read(int) = 0;
};
extern "C" __declspec(dllexport) int __cdecl RA2Smoke_Read(Interface* p,int value) {
    return p->read(value);
}
extern "C" __declspec(dllexport) int __stdcall RA2Smoke_Stdcall(int n) { return n+1; }
